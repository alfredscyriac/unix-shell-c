#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROMPT "CS340Shell% "
#define MAX_LINE 1024
#define MAX_ARGS 64

/* This function will return 1 if argv[0] was a built-in (exit, cd, or time) and 0 otherwise. */
static int run_builtin(char *argv[]) {
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }

    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = argv[1] ? argv[1] : getenv("HOME");
        if (dir == NULL || chdir(dir) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(argv[0], "time") == 0) {
        time_t now = time(NULL);
        printf("%s", ctime(&now));
        return 1;
    }

    return 0;
}

static void apply_redirections(const char *infile, const char *outfile) {
    if (infile != NULL) {
        int fd = open(infile, O_RDONLY);
        if (fd < 0) {
            perror(infile);
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (outfile != NULL) {
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror(outfile);
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

int main(void) {
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
        char *infile = NULL;
        char *outfile = NULL;
        char *save = NULL;
        
        char *tok = strtok_r(line, " \t", &save);
        while (tok != NULL) {
            if (strcmp(tok, "<") == 0) {
                infile = strtok_r(NULL, " \t", &save);
            } else if (strcmp(tok, ">") == 0) {
                outfile = strtok_r(NULL, " \t", &save);
            } else if (argc < MAX_ARGS - 1) {
                argv[argc++] = tok;
            }
            tok = strtok_r(NULL, " \t", &save);
        }
        argv[argc] = NULL;

        if (argc == 0) {
            continue;
        }

        if (run_builtin(argv)) {
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            apply_redirections(infile, outfile);
            execv(argv[0], argv);
            perror(argv[0]);
            _exit(127);
        }
        waitpid(pid, NULL, 0);
    }
    return 0;
}