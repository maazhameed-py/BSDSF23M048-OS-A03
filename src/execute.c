#include "shell.h"
#include <fcntl.h>

int execute(char* arglist[]) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    // Parse tokens into pipeline stages and per-stage redirections
    typedef struct {
        char *argv[MAXARGS + 1];
        char *infile;
        char *outfile;
    } stage_t;

    stage_t stages[MAXARGS]; // upper bound; practical pipelines are small
    int nst = 0;
    int argc = 0;

    // init first stage
    memset(&stages[0], 0, sizeof(stage_t));

    for (int i = 0; arglist[i] != NULL; i++) {
        char *t = arglist[i];
        if (strcmp(t, "|") == 0) {
            if (argc == 0) {
                fprintf(stderr, "myshell: invalid null command\n");
                return 0;
            }
            stages[nst].argv[argc] = NULL;
            nst++;
            if (nst >= MAXARGS) {
                fprintf(stderr, "myshell: pipeline too long\n");
                return 0;
            }
            memset(&stages[nst], 0, sizeof(stage_t));
            argc = 0;
        } else if (strcmp(t, "<") == 0) {
            if (arglist[i+1] == NULL) {
                fprintf(stderr, "myshell: syntax error near unexpected token '<'\n");
                return 0;
            }
            stages[nst].infile = arglist[++i];
        } else if (strcmp(t, ">") == 0) {
            if (arglist[i+1] == NULL) {
                fprintf(stderr, "myshell: syntax error near unexpected token '>'\n");
                return 0;
            }
            stages[nst].outfile = arglist[++i];
        } else {
            if (argc < MAXARGS) {
                stages[nst].argv[argc++] = t;
            }
        }
    }

    if (argc > 0) {
        stages[nst].argv[argc] = NULL;
        nst++;
    }

    if (nst == 0)
        return 0;

    // No pipe: handle possible builtin and redirections
    if (nst == 1) {
        if (handle_builtin(stages[0].argv))
            return 0;

        pid_t cpid = fork();
        if (cpid < 0) {
            perror("fork failed");
            return 0;
        }
        if (cpid == 0) {
            // child: setup redirections
            if (stages[0].infile) {
                int fd = open(stages[0].infile, O_RDONLY);
                if (fd < 0) { perror("open <"); _exit(1); }
                if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 <"); _exit(1); }
                close(fd);
            }
            if (stages[0].outfile) {
                int fd = open(stages[0].outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open >"); _exit(1); }
                if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 >"); _exit(1); }
                close(fd);
            }
            execvp(stages[0].argv[0], stages[0].argv);
            perror("Command not found");
            _exit(1);
        }
        int status;
        waitpid(cpid, &status, 0);
        return 0;
    }

    // Pipeline of nst stages
    int num_pipes = nst - 1;
    int pipes_arr[MAXARGS][2];
    for (int p = 0; p < num_pipes; p++) {
        if (pipe(pipes_arr[p]) < 0) {
            perror("pipe");
            // close any previously created
            for (int q = 0; q < p; q++) { close(pipes_arr[q][0]); close(pipes_arr[q][1]); }
            return 0;
        }
    }

    pid_t pids[MAXARGS];
    for (int si = 0; si < nst; si++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // parent cleanup: close all pipes
            for (int p = 0; p < num_pipes; p++) { close(pipes_arr[p][0]); close(pipes_arr[p][1]); }
            // wait for any previously forked children
            for (int k = 0; k < si; k++) waitpid(pids[k], NULL, 0);
            return 0;
        }
        if (pid == 0) {
            // child
            // connect stdin if not first
            if (si > 0) {
                if (dup2(pipes_arr[si-1][0], STDIN_FILENO) < 0) { perror("dup2 pipe in"); _exit(1); }
            }
            // connect stdout if not last
            if (si < nst - 1) {
                if (dup2(pipes_arr[si][1], STDOUT_FILENO) < 0) { perror("dup2 pipe out"); _exit(1); }
            }
            // close all pipe fds in child
            for (int p = 0; p < num_pipes; p++) { close(pipes_arr[p][0]); close(pipes_arr[p][1]); }

            // apply per-stage redirections (override pipe ends if specified)
            if (stages[si].infile) {
                int fd = open(stages[si].infile, O_RDONLY);
                if (fd < 0) { perror("open <"); _exit(1); }
                if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 <"); _exit(1); }
                close(fd);
            }
            if (stages[si].outfile) {
                int fd = open(stages[si].outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open >"); _exit(1); }
                if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 >"); _exit(1); }
                close(fd);
            }

            execvp(stages[si].argv[0], stages[si].argv);
            perror("Command not found");
            _exit(1);
        } else {
            pids[si] = pid;
        }
    }

    // parent: close all pipe fds
    for (int p = 0; p < num_pipes; p++) { close(pipes_arr[p][0]); close(pipes_arr[p][1]); }

    // wait for all children
    for (int si = 0; si < nst; si++) {
        int status;
        waitpid(pids[si], &status, 0);
    }
    return 0;
}


  
