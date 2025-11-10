#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

// Readline
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512 
#define MAXARGS 10
#define ARGLEN 256  // Increased to accommodate longer variable values and quoted strings
#define PROMPT "Maaz-OS-A03> "

// Background jobs
#define MAXJOBS 64
typedef struct {
	pid_t pid;
	char  cmd[MAX_LEN];
} job_t;

// Function prototypes
char** tokenize(char* cmdline);
int execute(char** arglist, int background, const char* raw_cmd);
int handle_builtin(char **args);

// Jobs management
void add_job(pid_t pid, const char* cmd);
void remove_job(pid_t pid);
void print_jobs(void);
void reap_background(void);

// History display helper (implemented using GNU Readline history)
void history_print(void);

// Readline completion hook
char** myshell_completion(const char* text, int start, int end);

// Variables feature (v8)
typedef struct var_s {
	char *name;
	char *value;
	struct var_s *next;
} var_t;

// Variable management
void set_var(const char *name, const char *value);
const char* get_var(const char *name);
void print_vars(void);
void expand_variables(char **arglist);
void process_assignments(char **arglist); // removes assignment tokens and stores them

#endif // SHELL_H






