#include "shell.h"

int main() {
    // Set custom completion function
    rl_attempted_completion_function = myshell_completion;

    while (1) {
        // Reap any finished background jobs before showing prompt
        reap_background();
        char *cmdline = readline(PROMPT);
        if (!cmdline) break; // EOF (Ctrl-D)
        if (cmdline[0] == '\0') { free(cmdline); continue; }

        add_history(cmdline);

        // History expansion (!n)
        if (cmdline[0] == '!') {
            char *p = cmdline + 1;
            while (*p == ' ' || *p == '\t') p++;
            char *endptr = NULL; errno = 0;
            long n = strtol(p, &endptr, 10);
            if (p == endptr || errno != 0 || n <= 0 || n > history_length) {
                fprintf(stderr, "myshell: !%ld: event not found\n", n);
                free(cmdline); continue;
            }
            HIST_ENTRY *he = history_get((int)n);
            if (!he) { fprintf(stderr, "myshell: !%ld: event not found\n", n); free(cmdline); continue; }
            char *expanded = strdup(he->line);
            if (!expanded) { perror("strdup"); free(cmdline); continue; }
            printf("%s\n", expanded);
            free(cmdline); cmdline = expanded;
        }

        // Split on semicolons for command chaining
        char *work = strdup(cmdline);
        if (!work) { perror("strdup"); free(cmdline); continue; }
        char *saveptr = NULL;
        char *segment = strtok_r(work, ";", &saveptr);
        while (segment) {
            // Trim leading/trailing whitespace
            while (*segment == ' ' || *segment == '\t') segment++;
            char *end = segment + strlen(segment);
            while (end > segment && (end[-1] == ' ' || end[-1] == '\t')) { end--; }
            *end = '\0';
            if (*segment == '\0') { segment = strtok_r(NULL, ";", &saveptr); continue; }

            // Check for background '&' at end
            int background = 0;
            char *bend = segment + strlen(segment);
            // skip trailing spaces before '&'
            char *scan = bend - 1;
            while (scan >= segment && (*scan == ' ' || *scan == '\t')) scan--;
            if (scan >= segment && *scan == '&') {
                background = 1;
                // remove '&' and any whitespace before next parse
                *scan = '\0';
                // trim again trailing spaces
                char *e2 = scan; while (e2 > segment && (e2[-1] == ' ' || e2[-1] == '\t')) { e2--; } *e2 = '\0';
            }

            if (*segment != '\0') {
                char *cmd_copy = strdup(segment); // for job display
                if (!cmd_copy) { perror("strdup"); }
                char **arglist = tokenize(segment);
                if (arglist) {
                    // Handle variable assignments (built-in, no fork) and expand $VARS
                    process_assignments(arglist);
                    expand_variables(arglist);
                    execute(arglist, background, cmd_copy ? cmd_copy : segment);
                    for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
                    free(arglist);
                }
                if (cmd_copy) free(cmd_copy);
            }

            segment = strtok_r(NULL, ";", &saveptr);
        }
        free(work);
        free(cmdline);
    }
    printf("\nShell exited.\n");
    return 0;
}