#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    // Set custom completion function
    rl_attempted_completion_function = myshell_completion;

    while ((cmdline = readline(PROMPT)) != NULL) {
        if (cmdline[0] == '\0') { free(cmdline); continue; }

        // Add to readline history
        add_history(cmdline);

        // Handle !n expansion using readline's history list
        if (cmdline[0] == '!') {
            char *p = cmdline + 1;
            while (*p == ' ' || *p == '\t') p++;
            char *endptr = NULL;
            errno = 0;
            long n = strtol(p, &endptr, 10);
            if (p == endptr || errno != 0 || n <= 0 || n > history_length) {
                fprintf(stderr, "myshell: !%ld: event not found\n", n);
                free(cmdline);
                continue;
            }
            HIST_ENTRY *he = history_get((int)n);
            if (!he) {
                fprintf(stderr, "myshell: !%ld: event not found\n", n);
                free(cmdline);
                continue;
            }
            char *expanded = strdup(he->line);
            if (!expanded) { perror("strdup"); free(cmdline); continue; }
            printf("%s\n", expanded);
            free(cmdline);
            cmdline = expanded;
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            execute(arglist);

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}