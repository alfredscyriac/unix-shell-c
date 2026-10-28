#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROMPT     "CS340Shell% "
#define MAX_LINE   1024
#define MAX_ARGS   64
#define MAX_STAGES 32

/* One command in a pipeline */
typedef struct {
    char *argv[MAX_ARGS];
    char *infile;
    char *outfile;
} Command;

/* Splits one pipeline segment into words and < / > files. Returns total word count */
static int parse_command(char *segment, Command *cmd) {
    int argc = 0;
    char *save = NULL;

    cmd->infile = NULL;
    cmd->outfile = NULL;

    char *tok = strtok_r(segment, " \t", &save);
    while (tok != NULL) {
        if (strcmp(tok, "<") == 0) {
            cmd->infile = strtok_r(NULL, " \t", &save);
        } else if (strcmp(tok, ">") == 0) {
            cmd->outfile = strtok_r(NULL, " \t", &save);
        } else if (argc < MAX_ARGS - 1) {
            cmd->argv[argc++] = tok;
        }
        tok = strtok_r(NULL, " \t", &save);
    }
    cmd->argv[argc] = NULL;
    return argc;
}

/* Splits the line on every '|' and parses each piece individuallys. Returns the total command count */
static int parse_line(char *line, Command cmds[]) {
    int n = 0;
    char *save = NULL;

    char *segment = strtok_r(line, "|", &save);
    while (segment != NULL && n < MAX_STAGES) {
        if (parse_command(segment, &cmds[n]) > 0) {
            n++;
        }
        segment = strtok_r(NULL, "|", &save);
    }
    return n;
}

/* Runs exit, cd or time in the shell itself. Returns 1 if handled, else 0 */
static int run_builtin(const Command *cmd) {
    const char *name = cmd->argv[0];

    if (strcmp(name, "exit") == 0) {
        exit(0);
    }

    if (strcmp(name, "cd") == 0) {
        const char *dir = cmd->argv[1] ? cmd->argv[1] : getenv("HOME");
        if (dir == NULL || chdir(dir) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(name, "time") == 0) {
        time_t now = time(NULL);
        printf("%s", ctime(&now));
        return 1;
    }

    return 0;
}

/* Applies < and >, this is the same pattern from ExCode/03-IPC/redir.c */
static void apply_redirections(const Command *cmd) {
    if (cmd->infile != NULL) {
        int fd = open(cmd->infile, O_RDONLY);
        if (fd < 0) {
            perror(cmd->infile);
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->outfile != NULL) {
        int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror(cmd->outfile);
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

/*
 * Forks one child per command, connects them with pipes, and waits for all.
 * Pipe wiring follows pattern from ExCode/03-IPC/shell.c
 */
static void run_pipeline(Command cmds[], int n) {
    pid_t pids[MAX_STAGES];
    int started = 0;
    int prev_read = -1;                  

    for (int i = 0; i < n; i++) {
        int pipefd[2] = { -1, -1 };      

        if (i < n - 1 && pipe(pipefd) < 0) {
            perror("pipe");
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (pipefd[0] != -1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }
            break;
        }

        if (pid == 0) {
            if (prev_read != -1) {
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (pipefd[1] != -1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }
            apply_redirections(&cmds[i]);

            execv(cmds[i].argv[0], cmds[i].argv);
            perror(cmds[i].argv[0]);
            _exit(127);
        }

        pids[started++] = pid;
        if (prev_read != -1) {
            close(prev_read);
        }
        if (pipefd[1] != -1) {
            close(pipefd[1]);
            prev_read = pipefd[0];
        } else {
            prev_read = -1;
        }
    }

    if (prev_read != -1) {
        close(prev_read);
    }

    for (int i = 0; i < started; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

int main(void) {
    char line[MAX_LINE];
    Command cmds[MAX_STAGES];

    while (1) {
        fputs(PROMPT, stdout);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';

        int n = parse_line(line, cmds);
        if (n == 0) {
            continue;
        }

        if (n == 1 && run_builtin(&cmds[0])) {
            continue;
        }

        run_pipeline(cmds, n);
    }
    return 0;
}