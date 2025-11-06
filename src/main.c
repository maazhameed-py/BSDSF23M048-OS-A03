#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        // Handle !n before tokenization/history addition
        if (cmdline[0] == '!') {
            char *p = cmdline + 1;
            // allow optional spaces after '!'
            while (*p == ' ' || *p == '\t') p++;
            char *endptr = NULL;
            errno = 0;
            long n = strtol(p, &endptr, 10);
            if (p == endptr || errno != 0 || n <= 0) {
                fprintf(stderr, "myshell: invalid history reference: %s\n", cmdline);
                free(cmdline);
                continue;
            }
            const char *h = history_get((int)n);
            if (!h) {
                fprintf(stderr, "myshell: !%ld: event not found\n", n);
                free(cmdline);
                continue;
            }
            // replace cmdline with historical command
            free(cmdline);
            cmdline = strdup(h);
            if (!cmdline) {
                perror("strdup");
                continue;
            }
            printf("%s\n", cmdline); // echo the expanded command
        }

        // Add the (possibly expanded) command to history
        history_add(cmdline);

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