#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROMPT "CS340Shell% "
#define MAX_LINE 1024
#define MAX_ARGS 64

int main(void){
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    while(1) {
        fputs(PROMPT, stdout);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line,"\n")] = '\0';

        int argc = 0;
        char *save = NULL;
        char *tok = strtok_r(line, " \t", &save);
        while (tok != NULL && argc < MAX_ARGS - 1) {
            argv[argc++] = tok;
            tok = strtok_r(NULL, " \t", &save);
        }
        argv[argc] = NULL;

        if (argc == 0) {
            continue;
        }
        if (strcmp(line,"exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            execv(argv[0], argv);
            perror(argv[0]);
            _exit(127);
        }

        waitpid(pid, NULL, 0);
    }

    return 0;
}