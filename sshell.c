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
    while (1) {
        struct Input piping[CMDLINE_MAX];
        char tempCmd[CMDLINE_MAX];
        int commandCount = 0;

        char *nl;
        //int retval;

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

        int status;
        pid_t pid;
        int fds[2*commandCount];

        for(int i = 0; i < commandCount; i++) {
            parseInput(&piping[i]);
            checkRedirect(&piping[i]);
            if (pipe(fds + i*2)) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
        int outfd;

        for (int i = 0; i < commandCount; i++) {
            pid = fork();
            if (pid == 0) {
                /* Child */
                if (piping[i].willRedirect == 1) {
                    outfd = open(piping[i].file, O_WRONLY | O_CREAT, 0644);
                    dup2(outfd, STDOUT_FILENO);
                }
                if (i != 0) {
                    dup2(fds[(i-1)*2], 0);
                }
                if (i != commandCount-1) {
                    dup2(fds[(i*2)+1], 1);
                }
                execvp(piping[i].cmd, piping[i].args);
                for (int k = 0; k < 2*commandCount; k++) {
                    close(fds[k]);
                }
                perror("execvp");
                exit(EXIT_SUCCESS);
            } else if (pid > 0) {
                /* Parent */
                if (strcmp(piping[i].cmd, "cd") == 0) {
                    chdir(piping[i].args[1]);
                    printf("cd \n");
                }
                waitpid(pid, &status, 0);
            } else {
                /* Error forking */
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }
        for (int i = 0; i < 2*commandCount; i++) {
            close(fds[i]);
        }
        printf("+ completed '%s' [%d] \n", message, WEXITSTATUS(status));
    }
        return EXIT_SUCCESS;
}
