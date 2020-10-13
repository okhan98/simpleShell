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
    int willRedirect;
};

void parsePipe(struct Input input[CMDLINE_MAX], char raw[CMDLINE_MAX], int *commandCount) {
    /* String Tokenizer */
    int counter = 0;
    char *token = strtok(raw, "|");
    while(token != NULL) {
        /* Remove trailing whitespace from command line */
        if(token[strlen(token)-1] == ' ') {
            token[strlen(token)-1] = '\0';
        }
        /* Remove leading whitespace from command line */
        if(token[0] == ' '){
            token++;
        }
        struct Input temp;
        strcpy(temp.cmd, token);
        input[counter] = temp;
        token = strtok(NULL, "|");
        counter++;
        *commandCount = *commandCount + 1;
    }
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

void checkRedirect(struct Input *input) {
    for (size_t i = 0; input->args[i] != NULL; i++) {
        /* Check that the meta-character is not in the middle of the command, with no whitespace */
        for(size_t k = 0; k < strlen(input->args[i]); k++) {
            if (input->args[i][k] == '>') {
                if (k !=0 && k != strlen(input->args[i])-1) {
                    size_t length = strlen(input->args[i]);
                    char currToken[length];
                    strcpy(currToken, input->args[i]);
                    char *newcmd = strtok(input->args[i], ">");
                    input->args[i] = newcmd;
                    newcmd = strtok(NULL, ">");
                    strcpy(input->file, newcmd);
                    input->willRedirect = 1;
                    size_t argLen = strlen(*input->args);
                    for (size_t j = i; j < argLen; j++) {
                        if (input->args[j+1] != NULL && input->args[j+2] != NULL) {
                            input->args[j+1] = input->args[j+2];
                            input->args[strlen(*input->args)-1] = NULL;
                        }
                    }
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
                            if (input->args[j+1] != NULL) {
                                input->args[j] = input->args[j+1];
                                input->args[strlen(*input->args)-1] = NULL;
                            } else {
                                input->args[j] = NULL;
                                input->args[strlen(*input->args)-1] = NULL;
                            }
                        }
                } else if (input->args[i][1] == '\0') {
                        /* File name has a whitespace next to it */
                        strcpy(input->file, input->args[i+1]);
                        for (size_t j = i; j < argsLen; j++) {
                            if (input->args[j+2] != NULL) {
                                input->args[j] = input->args[j+2];
                                input->args[strlen(*input->args)-1] = NULL;
                            } else {
                                input->args[j] = NULL;
                                input->args[strlen(*input->args)-1] = NULL;
                            }
                        }
                }
                input->willRedirect = 1;
        } else if (input->args[i][tokenLen-1] == '>') { // meta-character is at end of token
                strcpy(input->file, input->args[i+1]); // File name should be the next argument
                input->args[i][tokenLen-1] = '\0'; // Removes the meta-character from the argument
                for (size_t j = i; j < argsLen; j++) {
                        input->args[j+1] = input->args[j+2];
                        input->args[strlen(*input->args)-1] = NULL;
                }
                input->willRedirect = 1;
        }
    }
};

int main(void)
{
    struct Input piping[CMDLINE_MAX];
    char tempCmd[CMDLINE_MAX];
    int commandCount = 0;
    while (1) {

        char *nl;
        int retval;

        pid_t pid;

        /* Print prompt */
        printf("sshell$ ");
        fflush(stdout);

        /* Get command line */
        fgets(tempCmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", tempCmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(tempCmd, '\n');
        if (nl)
            *nl = '\0';

        /* Builtin command */
        if (!strcmp(tempCmd, "exit")) {
            fprintf(stderr, "Bye...\n");
            break;
        }

        char message[CMDLINE_MAX];
        strcpy(message, tempCmd);

        /* Send the raw input to be parsed for piping */
        parsePipe(piping, tempCmd, &commandCount);
        printf("[DEV]: commandCount: %d \n",commandCount);

        /* Regular command */
        for(int i = 0; i < commandCount; i++){
            parseInput(&piping[i]);
            checkRedirect(&piping[i]);

            /* Remove this later, for debugging */
            printf("[DEV]: current command: %s \n", piping[i].cmd);

            pid = fork();

            if (pid == 0) {
                // Child
                int fd;
                if (piping[i].willRedirect == 1) {
                    fd = open(piping[i].file, O_WRONLY | O_CREAT, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                retval = execvp(piping[i].cmd, piping[i].args);
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                // Parent

                /* Change directory commands */
                if (strcmp(piping[i].cmd, "cd") == 0) {
                    chdir(*piping[i].args);
                }

                int status;
                waitpid(pid, &status, 0);
                printf("+ completed '%s' [%d] \n", message, WEXITSTATUS(status));

            } else {
                perror("fork");
                exit(1);
            }

        }
        commandCount = 0;
    }
        return EXIT_SUCCESS;
}
