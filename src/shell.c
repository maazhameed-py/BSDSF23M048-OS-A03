#include "shell.h"
// No heavy scanning for PATH to keep completion responsive

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
static const char* builtin_cmds[] = { "cd", "pwd", "help", "exit", "jobs", "history", NULL };

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
    for (int i = 0; i < MAXARGS + 1; i++)
    {
        arglist[i] = (char*) malloc(sizeof(char) * ARGLEN);
        memset(arglist[i], 0, ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS)
    {
        while (*cp == ' ' || *cp == '\t')
            cp++;  // Skip whitespace
        if (*cp == '\0')
            break;

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t'))
            len++;

        // Clamp token length to ARGLEN-1 to avoid buffer overflow
        int copy_len = len < (ARGLEN - 1) ? len : (ARGLEN - 1);
        strncpy(arglist[argnum], start, copy_len);
        arglist[argnum][copy_len] = '\0';
        argnum++;
    }

    if (argnum == 0)
    {
        for (int i = 0; i < MAXARGS + 1; i++)
            free(arglist[i]);
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
        return 1;
    }

    /* jobs (placeholder) */
    else if (strcmp(args[0], "jobs") == 0)
    {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    /* history */
    else if (strcmp(args[0], "history") == 0)
    {
        history_print();
        return 1;
    }

    return 0;  // not a builtin
}
