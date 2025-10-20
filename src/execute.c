#include "shell.h"

int execute(char* arglist[]) {
    int status;
	pid_t cpid;
	
	if (handle_builtin(arglist))
       return 0;
   
    cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0: // Child process
            execvp(arglist[0], arglist);
            perror("Command not found"); // This line runs only if execvp fails
            exit(1);
        default: // Parent process
            waitpid(cpid, &status, 0);
            // printf("Child pid:%d exited with status %d\n", cpid, status >> 8);
            return 0;
    }
}


  
