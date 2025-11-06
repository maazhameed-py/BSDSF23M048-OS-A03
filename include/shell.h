#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "OS-A03> "

// History configuration
#ifndef HISTORY_SIZE
#define HISTORY_SIZE 20
#endif

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char **args);

// History APIs
void history_add(const char* cmd);
const char* history_get(int n);   // 1-based index; returns NULL if out of range
int history_count(void);
void history_print(void);

#endif // SHELL_H






