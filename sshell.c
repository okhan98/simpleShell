#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16
#define TOKEN_MAX 32

struct Input{
    char cmd[CMDLINE_MAX];
    char args[ARGS_MAX];
    char tokens[TOKEN_MAX];
};

int main(void)
{
        struct Input input;

        while (1) {
                char *nl;
                int retval;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(input.cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", input.cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(input.cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(input.cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Regular command */
                retval = system(input.cmd);
                fprintf(stdout, "Return status value for '%s': %d\n",
                        input.cmd, retval);
        }

        return EXIT_SUCCESS;
}
