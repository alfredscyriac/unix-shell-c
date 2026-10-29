# CS340Shell

A small Unix shell written in C.

## Build and run

```
gcc -Wall -Wextra -o shell shell.c
./shell
```

## Features

- Runs programs by full path using `fork`, `execv` and `waitpid`
- Built-ins: `exit`, `cd`, `time`
- `<` and `>` redirection
- Any number of pipes, e.g. `/bin/ls -l | /bin/sort | /usr/bin/wc -l`

## Limitations

- Programs must be given by full path
- Tokens (including `<`, `>` and `|`) must be space-separated
- No quoting, background jobs, or stderr redirection
- Built-ins ignore redirection and don't work inside pipes
- Lines over 1023 characters are split into separate commands
- Pipelines longer than 32 commands are silently truncated
