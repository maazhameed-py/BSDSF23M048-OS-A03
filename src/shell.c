#include "shell.h"
// No heavy scanning for PATH to keep completion responsive

/* ------------ Background jobs list ------------ */
static job_t jobs[MAXJOBS];
static int   jobs_count = 0;

/* ------------ Variables (v8) ------------ */
static var_t *vars_head = NULL;

static int is_valid_var_start(char c) {
    return (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int is_valid_var_char(char c) {
    return is_valid_var_start(c) || (c >= '0' && c <= '9');
}

void set_var(const char *name, const char *value)
{
    if (!name) return;
    // find existing
    for (var_t *v = vars_head; v; v = v->next) {
        if (strcmp(v->name, name) == 0) {
            free(v->value);
            v->value = strdup(value ? value : "");
            return;
        }
    }
    // new
    var_t *nv = (var_t*)calloc(1, sizeof(var_t));
    if (!nv) { perror("calloc"); return; }
    nv->name = strdup(name);
    nv->value = strdup(value ? value : "");
    nv->next = vars_head;
    vars_head = nv;
}

const char* get_var(const char *name)
{
    if (!name) return "";
    for (var_t *v = vars_head; v; v = v->next) {
        if (strcmp(v->name, name) == 0) return v->value ? v->value : "";
    }
    return ""; // undefined expands to empty
}

void print_vars(void)
{
    for (var_t *v = vars_head; v; v = v->next) {
        printf("%s=%s\n", v->name, v->value ? v->value : "");
    }
}

static int is_assignment_token(const char *tok)
{
    if (!tok) return 0;
    // NAME=VALUE form; NAME must be [A-Za-z_][A-Za-z0-9_]*
    const char *eq = strchr(tok, '=');
    if (!eq) return 0;
    if (eq == tok) return 0; // starts with '=' not allowed
    // validate name
    const char *p = tok;
    if (!is_valid_var_start(*p)) return 0;
    p++;
    while (p < eq) {
        if (!is_valid_var_char(*p)) return 0;
        p++;
    }
    return 1;
}

void process_assignments(char **arglist)
{
    if (!arglist) return;
    for (int i = 0; arglist[i] != NULL; ) {
        char *tok = arglist[i];
        if (is_assignment_token(tok)) {
            // split into name and value
            char *eq = strchr(tok, '=');
            *eq = '\0';
            const char *name = tok;
            const char *value = eq + 1;
            set_var(name, value);
            // remove token i
            free(tok);
            int j = i;
            while (arglist[j] != NULL) {
                arglist[j] = arglist[j+1];
                j++;
            }
            // do not increment i; now points to next element
            continue;
        }
        i++;
    }
}

void expand_variables(char **arglist)
{
    if (!arglist) return;
    for (int i = 0; arglist[i] != NULL; i++) {
        char *tok = arglist[i];
        if (!tok || tok[0] == '\0') continue;
        // skip operator tokens
        if ((tok[0] == '|' || tok[0] == '<' || tok[0] == '>') && tok[1] == '\0') continue;

        if (tok[0] == '$') {
            const char *name = tok + 1;
            if (*name == '\0') continue; // lone '$'
            // parse var name letters/digits/_ only
            int k = 0;
            while (name[k] && is_valid_var_char(name[k])) k++;
            // Only expand if token is exactly $NAME (no suffix)
            if (name[k] == '\0') {
                const char *val = get_var(name);
                char *rep = strdup(val ? val : "");
                if (!rep) { perror("strdup"); continue; }
                free(arglist[i]);
                arglist[i] = rep;
            }
        }
    }
}

void add_job(pid_t pid, const char* cmd)
{
    if (pid <= 0) return;
    if (jobs_count >= MAXJOBS) {
        fprintf(stderr, "myshell: job table full, cannot track PID %d\n", (int)pid);
        return;
    }
    jobs[jobs_count].pid = pid;
    strncpy(jobs[jobs_count].cmd, cmd ? cmd : "(unknown)", sizeof(jobs[jobs_count].cmd) - 1);
    jobs[jobs_count].cmd[sizeof(jobs[jobs_count].cmd) - 1] = '\0';
    jobs_count++;
}

void remove_job(pid_t pid)
{
    for (int i = 0; i < jobs_count; i++) {
        if (jobs[i].pid == pid) {
            // compact
            for (int j = i + 1; j < jobs_count; j++) jobs[j-1] = jobs[j];
            jobs_count--;
            return;
        }
    }
}

void print_jobs(void)
{
    if (jobs_count == 0) {
        printf("(no background jobs)\n");
        return;
    }
    for (int i = 0; i < jobs_count; i++) {
        printf("[%d] %d  %s\n", i + 1, (int)jobs[i].pid, jobs[i].cmd);
    }
}

void reap_background(void)
{
    int status;
    pid_t pid;
    // Reap all finished children without blocking
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Optional: uncomment for verbose completion logging:
        // if (WIFEXITED(status)) {
        //     fprintf(stderr, "[bg] pid %d exited with %d\n", (int)pid, WEXITSTATUS(status));
        // } else if (WIFSIGNALED(status)) {
        //     fprintf(stderr, "[bg] pid %d killed by signal %d\n", (int)pid, WTERMSIG(status));
        // }
        remove_job(pid);
    }
}

/* ------------ History printing via Readline ------------ */
void history_print(void)
{
    if (history_length == 0) {
        printf("(history empty)\n");
        return;
    }
    HIST_ENTRY **list = history_list();
    if (!list) {
        printf("(history unavailable)\n");
        return;
    }
    for (int i = 0; list[i] != NULL; i++) {
        printf("%d  %s\n", i + 1, list[i]->line);
    }
}

/* ------------ Readline completion (built-ins + default filenames) ------------ */
static const char* builtin_cmds[] = { "cd", "pwd", "help", "exit", "jobs", "history", "set", NULL };

static char* builtin_generator(const char* text, int state)
{
    static int idx;
    size_t len = strlen(text);
    if (state == 0) idx = 0;
    while (builtin_cmds[idx]) {
        const char* cand = builtin_cmds[idx++];
        if (strncmp(cand, text, len) == 0)
            return strdup(cand);
    }
    return NULL;
}

char** myshell_completion(const char* text, int start, int end)
{
    (void)end;
    // At start of line, complete built-ins. If user typed a path component, let default filename completion handle it.
    if (start == 0) {
        if (strchr(text, '/')) return NULL; // fallback to default filename completion
        return rl_completion_matches(text, builtin_generator);
    }
    // Non-first word: fall back to default filename completion (NULL tells readline to do default)
    return NULL;
}

/* ------------ Tokenize command string ------------ */
char** tokenize(char* cmdline)
{
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n')
        return NULL;

    char** arglist = (char**) malloc(sizeof(char*) * (MAXARGS + 1));
    if (!arglist) { perror("malloc"); return NULL; }
    for (int i = 0; i < MAXARGS + 1; i++) arglist[i] = NULL;

    int argnum = 0;
    const char *cp = cmdline;

    while (*cp != '\0' && argnum < MAXARGS)
    {
        // Skip whitespace
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0' || *cp == '\n') break;

        // If special operator, emit as its own token
        if (*cp == '<' || *cp == '>' || *cp == '|') {
            char *tok = (char*)calloc(2, 1);
            if (!tok) { perror("calloc"); break; }
            tok[0] = *cp;
            tok[1] = '\0';
            arglist[argnum++] = tok;
            cp++;
            continue;
        }

        // Otherwise, read a word possibly containing quoted substrings
        char buf[ARGLEN];
        int len = 0;
        while (*cp != '\0' && *cp != '\n' && *cp != ' ' && *cp != '\t' && *cp != '<' && *cp != '>' && *cp != '|') {
            if (*cp == '"') {
                cp++; // skip opening quote
                while (*cp != '\0' && *cp != '"') {
                    if (len < ARGLEN - 1) buf[len++] = *cp;
                    cp++;
                }
                if (*cp == '"') cp++; // skip closing quote
            } else if (*cp == '\'') {
                cp++; // skip opening single quote
                while (*cp != '\0' && *cp != '\'') {
                    if (len < ARGLEN - 1) buf[len++] = *cp;
                    cp++;
                }
                if (*cp == '\'') cp++; // skip closing single quote
            } else {
                if (len < ARGLEN - 1) buf[len++] = *cp;
                cp++;
            }
        }
        buf[len] = '\0';
        if (len > 0) {
            arglist[argnum] = strdup(buf);
            if (!arglist[argnum]) { perror("strdup"); break; }
            argnum++;
        }
    }

    if (argnum == 0)
    {
        for (int i = 0; i < MAXARGS + 1; i++)
            if (arglist[i]) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* ------------ Handle built-in shell commands ------------ */
int handle_builtin(char **args)
{
    if (args == NULL || args[0] == NULL)
        return 0;

    /* exit */
    if (strcmp(args[0], "exit") == 0)
    {
        printf("Exiting myshell...\n");
        exit(0);
    }

    /* cd */
    else if (strcmp(args[0], "cd") == 0)
    {
        if (args[1] == NULL)
            fprintf(stderr, "myshell: expected argument to \"cd\"\n");
        else if (chdir(args[1]) != 0)
            perror("myshell");
        return 1;
    }

    /* pwd */
    else if (strcmp(args[0], "pwd") == 0)
    {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("myshell");
        return 1;
    }

    /* help */
    else if (strcmp(args[0], "help") == 0)
    {
        printf("myshell built-in commands:\n");
        printf("  cd [dir]   - change directory\n");
        printf("  pwd        - print current working directory\n");
        printf("  help       - show this help message\n");
        printf("  exit       - exit the shell\n");
        printf("  jobs       - (not yet implemented)\n");
        printf("  history    - show command history\n");
        printf("  !n         - re-execute nth command from history\n");
        printf("  set        - list shell variables\n");
        return 1;
    }

    /* jobs (placeholder) */
    else if (strcmp(args[0], "jobs") == 0)
    {
        print_jobs();
        return 1;
    }

    /* history */
    else if (strcmp(args[0], "history") == 0)
    {
        history_print();
        return 1;
    }

        /* set (list variables) */
        else if (strcmp(args[0], "set") == 0)
        {
            print_vars();
            return 1;
        }

    return 0;  // not a builtin
}
