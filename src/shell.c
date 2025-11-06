#include "shell.h"

/* ------------ History Data Structure ------------ */
static char* history[HISTORY_SIZE];
static int history_head = 0;      // points to next slot to write
static int history_len = 0;       // number of stored commands (<= HISTORY_SIZE)

void history_add(const char* cmd)
{
    if (cmd == NULL || cmd[0] == '\0') return;
    // Duplicate command string
    char* copy = strdup(cmd);
    if (!copy) return; // Allocation failure: silently ignore

    // If overwriting existing entry, free it first
    if (history_len == HISTORY_SIZE && history[history_head] != NULL) {
        free(history[history_head]);
    }

    history[history_head] = copy;
    history_head = (history_head + 1) % HISTORY_SIZE;
    if (history_len < HISTORY_SIZE) history_len++;
}

const char* history_get(int n)
{
    if (n < 1 || n > history_len) return NULL;
    // Oldest entry index
    int start = (history_head - history_len + HISTORY_SIZE) % HISTORY_SIZE;
    int idx = (start + (n - 1)) % HISTORY_SIZE;
    return history[idx];
}

int history_count(void) { return history_len; }

void history_print(void)
{
    if (history_len == 0) {
        printf("(history empty)\n");
        return;
    }
    int start = (history_head - history_len + HISTORY_SIZE) % HISTORY_SIZE;
    for (int i = 0; i < history_len; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        printf("%d  %s\n", i + 1, history[idx]);
    }
}

/* ------------ Read a command from user ------------ */
char* read_cmd(char* prompt, FILE* fp)
{
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF)
    {
        if (c == '\n')
            break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0)
    {
        free(cmdline);
        return NULL;  // Handle Ctrl+D (end of input)
    }

    cmdline[pos] = '\0';
    return cmdline;
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
        bzero(arglist[i], ARGLEN);
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

        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
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
