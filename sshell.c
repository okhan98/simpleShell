#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16
#define TOKEN_MAX 32

struct Input{
    char cmd[CMDLINE_MAX];
    char *args[ARGS_MAX];
    char tokens[TOKEN_MAX];
    char file[TOKEN_MAX];
};

void parseInput(struct Input *input) {
    /* String Tokenizer */
    char *token = strtok(input->cmd, " ");
    int counter = 0;
    while(token != NULL)
    {
        if (counter == 0) {
            input->cmd[0] = *token;
            input->args[0] = token;
        }
        if (counter > 0) {
            input->args[counter] = token;
        }
        token = strtok(NULL, " ");
        counter++;
    }
    input->args[counter] = NULL; // Sets last argument to NULL to prevent whitespace/empty argument issues
};

int checkRedirect(struct Input *input) {
    int willRedirect = 0;
    for (int i = 0; input->args[i] != NULL; i++) {
            for(size_t k = 0; k < strlen(input->args[i]); k++) {
                if (input->args[i][k] == '>') {
                    if (k !=0 && k != strlen(input->args[i])-1) {
                        printf("Found redirection character in middle of token at position: %lu \n", k);
                    }
                }
            }

            size_t argsLen = strlen(*input->args);
            size_t tokenLen = strlen(input->args[i]);

            /* meta-character is at the beginning of the token */
            if (input->args[i][0] == '>') {
                    if (input->args[i][1] != '\0') {
                            /* File name does not have a whitespace next to it */
                            char *name = input->args[i] + 1;
                            char fileName[strlen(name) + 1];
                            strcpy(fileName, name);
                            strcpy(input->file, fileName);
                            /* Shift the char array to remove the file name argument */
                            for (size_t j = i; j < argsLen; j++) {
                                    input->args[j] = input->args[j+1];
                                    input->args[strlen(*input->args)] = NULL;
                            }
                    } else if (input->args[i][1] == '\0') {
                            /* File name has a whitespace next to it */
                            strcpy(input->file, input->args[i+1]);
                            for (size_t j = i; j < argsLen; j++) {
                                input->args[j] = input->args[j+2];
                                input->args[strlen(*input->args)] = NULL;
                            }
                    }
                    willRedirect = 1;
            } else if (input->args[i][tokenLen-1] == '>') {
                    strcpy(input->file, input->args[i+1]); // File name should be the next argument
                    input->args[i][tokenLen-1] = '\0'; // Removes the meta-character from the argument
                    for (size_t j = i; j < argsLen; j++) {
                            input->args[j+1] = input->args[j+2];
                            input->args[strlen(*input->args)] = NULL;
                    }
                    willRedirect = 1;
            }
  }
  return willRedirect;
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

                /* Send the input to be parsed for arguments */
                parseInput(&input);
                int willRedirect = checkRedirect(&input);

                /* Remove this later, for debugging */
                printf("[DEV]: list of arguments: \n");
                for (int i = 0; input.args[i] != NULL; i++) {
                    printf("%s \n", input.args[i]);
                }

                /* Regular command */
                pid = fork();

                if(pid == 0){
                    // Child
                    int fd;
                    if (willRedirect == 1) {
                        fd = open(input.file, O_WRONLY | O_CREAT, 0644);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
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
