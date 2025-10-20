#include "shell.h"

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
        return 1;
    }

    /* jobs (placeholder) */
    else if (strcmp(args[0], "jobs") == 0)
    {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0;  // not a builtin
}

/* ------------ Main shell loop (example) ------------ */
void run_shell(void)
{
    char* cmdline;
    char** arglist;

    while (1)
    {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("\033[1;32mmyshell\033[0m:\033[1;34m%s\033[0m$ ", cwd);
        fflush(stdout);

        cmdline = read_cmd("", stdin);
        if (cmdline == NULL)
            break;  // Ctrl+D exits shell

        arglist = tokenize(cmdline);
        if (arglist == NULL)
        {
            free(cmdline);
            continue;
        }

        /* Built-ins and external commands handled in execute() */
        execute(arglist);

        free(cmdline);
        for (int i = 0; i < MAXARGS + 1; i++)
            free(arglist[i]);
        free(arglist);
    }

    printf("\nExiting myshell...\n");
}
