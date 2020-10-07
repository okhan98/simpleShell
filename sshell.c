#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16
#define TOKEN_MAX 32

struct Input{
    char cmd[CMDLINE_MAX];
    char *args[ARGS_MAX];
    char tokens[TOKEN_MAX];
};

int main(void)
{
    struct Input input;
    while (1) {
                char *nl;
                int retval;

                pid_t pid;

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
                char message[CMDLINE_MAX];
                strcpy(message, input.cmd);
                printf("message: %s \n", message);

                // String Tokenizer
                char *token = strtok(input.cmd, " ");
                int counter = 0;
                while(token != NULL)
                {
                    if(counter == 0)
                    {
                        input.cmd[0] = *token;
                    }
                    if(counter > 0)
                    {
                        for(int i = 0; i < ARGS_MAX; i++)
                            input.args[i] = token;
                    }
                    token = strtok(NULL, " ");
                    counter = counter + 1;
                }
                /* Regular command */
                input.args[counter] = NULL; // Resets args
                pid = fork();

                if(pid == 0){
                    // Child
                    retval = execvp(input.cmd, input.args);
                    perror("execvp");
                    exit(1);
                }
                else if(pid > 0) {

                    // Parent

                    // Change directory
                    if(strcmp(input.cmd,"cd") == 0)
                    {
                        chdir(*input.args);
                    }

                    int status;
                    waitpid(pid, &status, 0);
                    printf("+ completed '%s' [%d] \n", message, WEXITSTATUS(status));

                }
                else{
                        perror("fork");
                        exit(1);
                    }
             //   fprintf(stdout, "Return status value for '%s': %d\n", input.cmd, retval);

        }

        return EXIT_SUCCESS;
}
