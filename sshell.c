#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
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
    int willAppend;
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
            if ((input->args[i][k] == '>') & (input->args[i][k+1] == '>')) {
                /* Meta-characters are in middle, will append. */
                if ((k != 0) && (k != strlen(input->args[i])-3) && (k != strlen(input->args[i])-2)) {
                    size_t length = strlen(input->args[i]);
                    char currToken[length];
                    strcpy(currToken, input->args[i]);
                    char *newcmd = strtok(input->args[i], ">>");
                    input->args[i] = newcmd;
                    newcmd = strtok(NULL, ">>");
                    strcpy(input->file, newcmd);
                    input->willRedirect = 1;
                    input->willAppend = 1;
                    size_t argLen = strlen(*input->args);
                    for (size_t j = i; j < argLen; j++) {
                        if (input->args[j+1] != NULL && input->args[j+2] != NULL) {
                            input->args[j+1] = input->args[j+2];
                            input->args[strlen(*input->args)-1] = NULL;
                        }
                    }
                }
            } else if ((input->args[i][k] == '>') && (input->args[i][k+1] != '>') && (input->args[i][k-1] != '\0') && (input->args[i][k-1] != '>')) {
                /* Meta-character in middle, but not appending. */
                if (k != 0 && k != strlen(input->args[i])-1) {
                    size_t length = strlen(input->args[i]);
                    char currToken[length];
                    strcpy(currToken, input->args[i]);
                    char *newcmd = strtok(input->args[i], ">");
                    input->args[i] = newcmd;
                    newcmd = strtok(NULL, ">");
                    strcpy(input->file, newcmd);
                    input->willRedirect = 1;
                    input->willAppend = 0;
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
                if ((input->args[i][1] != '\0') && (input->args[i][1] != '>')) {
                        /* File name does not have a whitespace next to it, should be the file name. */
                        char *name = input->args[i] + 1;
                        char fileName[strlen(name) + 1];
                        strcpy(fileName, name);
                        strcpy(input->file, fileName);
                        /* Shift the char array to remove the file name argument */
                        for (size_t j = i; j < strlen(*input->args); j++) {
                            if (input->args[j+1] != NULL) {
                                input->args[j] = input->args[j+1];
                                input->args[strlen(*input->args)-1] = NULL;
                            } else {
                                input->args[j] = NULL;
                                input->args[strlen(*input->args)-1] = NULL;
                            }
                        }
                        input->willAppend = 0;
                } else if (input->args[i][1] == '\0') {
                        /* File name has a whitespace next to it, next argument is file name. */
                        strcpy(input->file, input->args[i+1]);
                        for (size_t j = i; j < strlen(*input->args)-1; j++) {
                            if (input->args[j] != NULL) {
                                if ( (input->args[j+2] != NULL) && !(j >= strlen(*input->args)-1) ) {
                                    input->args[j] = input->args[j+2];
                                    input->args[j+2] = NULL;
                                } else {
                                    input->args[j] = NULL;
                                    input->args[j+1] = NULL;
                                }
                            }
                        }
                        input->args[strlen(*input->args)-1] = NULL;
                        input->willAppend = 0;
                } else if (input->args[i][1] == '>') {
                        /* Secondary meta-character, the output will be appended. */
                        if (input->args[i][2] != '\0') {
                            /* File name does not have a whitespace next to it, should be the file name. */
                            char *name = input->args[i] + 2;
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
                        } else if (input->args[i][2] == '\0') {
                            /* File name has a whitespace next to it, next argument is file name.*/
                            strcpy(input->file, input->args[i+1]);
                            for (size_t j = i; j < strlen(*input->args)-1; j++) {
                                if (input->args[j] != NULL) {
                                    if ( (input->args[j+2] != NULL) && !(j >= strlen(*input->args)-1) ) {
                                        input->args[j] = input->args[j+2];
                                        input->args[j+2] = NULL;
                                    } else {
                                        input->args[j] = NULL;
                                        input->args[j+1] = NULL;
                                    }
                                }
                            }
                            input->args[strlen(*input->args)-1] = NULL;
                        }
                        input->willAppend = 1;
                }
                input->willRedirect = 1;
        } else if ((input->args[i][tokenLen-2] == '>') && (input->args[i][tokenLen-1] == '>')) { // meta-characters are at end of token
                strcpy(input->file, input->args[i+1]); // File name should be the next argument
                input->args[i][tokenLen-2] = '\0'; // Removes the meta-characters from the argument
                for (size_t j = i; j < strlen(*input->args)-1; j++) {
                        if (input->args[j+2] != NULL) {
                            input->args[j+1] = input->args[j+2];
                        } else {
                            input->args[j+1] = NULL;
                        }
                }
                input->args[strlen(*input->args)-1] = NULL;
                input->willRedirect = 1;
                input->willAppend = 1;
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

void printCmdCompletion(char message[], int status) {
    printf("+ completed '%s' [%d] \n", message, status);
};

void executeCommands(struct Input piping[], int fds[], int commandCount, char message[]) {
    int status;
    pid_t pid;
    int outfd;

    for (int i = 0; i < commandCount; i++) {
        pid = fork();
        if (pid == 0) {
            /* Child */
            if (piping[i].willRedirect == 1) {
                if (piping[i].willAppend == 1) {
                    outfd = open(piping[i].file, O_WRONLY | O_APPEND | O_CREAT, 0644);
                } else {
                    outfd = open(piping[i].file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                }
                if (outfd != -1) {
                    dup2(outfd, STDOUT_FILENO);
                    close(outfd);
                } else {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
            }
            if (i != 0) {
                dup2(fds[(i-1)*2], 0);
            }
            if (i != commandCount-1) {
                dup2(fds[(i*2)+1], 1);
            }
            execvp(piping[i].cmd, piping[i].args);
            perror("execvp");
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            /* Parent */
            if (strcmp(piping[i].cmd, "cd") == 0) {
                chdir(piping[i].args[1]);
            }
            waitpid(pid, &status, 0);
        } else {
            /* Error forking */
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
    printCmdCompletion(message, WEXITSTATUS(status));
};

void execSLS(char message[]) {
    int count;
    struct dirent **files;
    char cwd[CMDLINE_MAX];
    struct stat *fileInfo[CMDLINE_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        count = scandir(cwd, &files, NULL, alphasort);
        if (count > 0) {
            for (int i = 2; i < count; i++) {
                fileInfo[i] = malloc(sizeof(struct stat));
                char filePath[CMDLINE_MAX];
                strcpy(filePath, cwd);
                strcat(filePath, "/");
                strcat(filePath, files[i]->d_name);
                int fileSize = 0;
                if (stat(filePath, fileInfo[i]) == 0) {
                    fileSize = fileInfo[i]->st_size;
                } else {
                    perror("stat");
                }
                printf("%s (%d bytes)\n", files[i]->d_name, fileSize);
            }
            printCmdCompletion(message, EXIT_SUCCESS);
        } else {
            printf("Error: cannot open directory \n");
            printCmdCompletion(message, EXIT_FAILURE);
        }
    } else {
        perror("getcwd");
        printCmdCompletion(message, EXIT_FAILURE);
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

        char message[CMDLINE_MAX];
        strcpy(message, tempCmd);

        /* Builtin and user command chain */
        if (!strcmp(tempCmd, "exit")) {
            fprintf(stderr, "Bye...\n");
            printCmdCompletion(message, EXIT_SUCCESS);
            break;
        } else if (!strcmp(tempCmd, "sls")) {
            execSLS(message);
        } else {
            /* Send the raw input to be parsed for piping */
            parsePipe(piping, tempCmd, &commandCount);

            /* Parse commands, create array of file descriptors, and execute commands */
            int fds[2*commandCount];
            for(int i = 0; i < commandCount; i++) {
                parseInput(&piping[i]);
                checkRedirect(&piping[i]);
                if (pipe(fds + i*2)) { // create pipes
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }
            executeCommands(piping, fds, commandCount, message);
        }
    }
    return EXIT_SUCCESS;
}
