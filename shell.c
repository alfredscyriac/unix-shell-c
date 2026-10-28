#include <stdio.h>
#include <string.h>

#define PROMPT "CS340Shell% "
#define MAX_LINE 1024

int main(void){
    char line[MAX_LINE];

    while(1) {
        fputs(PROMPT, stdout);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line,"\n")] = '\0';

        if (strcmp(line,"exit") == 0) {
            break;
        }
        printf("You typed: [%s]\n", line);
    }

    return 0;
}