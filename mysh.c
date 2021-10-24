/**
 * Copyright 2021 Isha Padmanaban
 * Name : Isha Padmanaban
 * Email : ipadmanaban@wisc.edu
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAXCMDSIZE 512

/**
 * Data structure to store alias names
 */
struct aliasNode {
    char *aliasOrig;                // Contains original command
    char *aliasMap;                 // Contains alias mapped command
    struct aliasNode *nextANode;    // To access next alias node
};

/**
 * Function to update existing alias with correct replacement
 */
int updateAliasNode(struct aliasNode* startANode,
                    char *aliasMapData, char *aliasOrigData) {
    struct aliasNode* travANode = startANode;
    while (travANode != NULL) {
        // Search if alias already present
        if (strcmp(travANode->aliasMap, aliasMapData) == 0) {
            // Search if alias mapped to correct replacement
            if (strcmp(travANode->aliasOrig, aliasOrigData) != 0) {
                // Update replacement command for alias mapping
                free(travANode->aliasOrig);
                travANode->aliasOrig = (char*)malloc(strlen(aliasOrigData)+1);
                strcpy(travANode->aliasOrig, aliasOrigData);
            }
            return 1;
        }
        travANode = travANode->nextANode;
    }
    return 0;
}

/**
 * Functuion to insert new alias mapping with replacement 
 */
void insertAliasNode(struct aliasNode** startANode,
                     char *aliasMapData, char *aliasOrigData) {
    struct aliasNode* newANode;
    newANode = (struct aliasNode*) malloc(sizeof(struct aliasNode));
    struct aliasNode *endANode = *startANode;

    // Memory allocation for alias mapping and replacement command
    newANode->aliasOrig = (char*)malloc(strlen(aliasOrigData)+1);
    newANode->aliasMap = (char*)malloc(strlen(aliasMapData)+1);

    // Copy alias mapping and its equivalent replacement command
    strcpy(newANode->aliasOrig, aliasOrigData);
    strcpy(newANode->aliasMap, aliasMapData);
    newANode->nextANode = NULL;

    // Check if empty linked list and add node
    if (*startANode == NULL) {
       *startANode = newANode;
       return;
    }

    // Find position to insert new alias node
    while (endANode->nextANode != NULL) {
        endANode = endANode->nextANode;
    }

    endANode->nextANode = newANode;

    return;
}

/**
 * Function to delete entire alias linked list
 */
void deleteAliasList(struct aliasNode** startANode) {
    struct aliasNode* travANode = *startANode;
    struct aliasNode* nextANode;

    // Delete each alias node in alias linked list
    while (travANode != NULL) {
        nextANode = travANode->nextANode;
        free(travANode->aliasOrig);
        free(travANode->aliasMap);
        free(travANode);
        travANode = nextANode;
    }

    *startANode = NULL;
}

/**
 * Function to delete alias node from alias linked list
 */
void deleteAliasNode(struct aliasNode** startANode, char *aliasMapData) {
    struct aliasNode *travANode = *startANode, *prevANode;

    // Delete first alias node of alias linked list if needed
    if (travANode != NULL && (strcmp(travANode->aliasMap, aliasMapData) == 0)) {
        *startANode = travANode->nextANode;
        free(travANode);
        return;
    }

    // Find the alias node to be deleted from alias linked list
    while (travANode != NULL &&
            (strcmp(travANode->aliasMap, aliasMapData) != 0)) {
        prevANode = travANode;
        travANode = travANode->nextANode;
    }

    // Do nothing if the node to be deleted is not found
    if (travANode == NULL) {
        return;
    }

    prevANode->nextANode = travANode->nextANode;

    // Free the memory allocated for alias node
    free(travANode);
}

/**
 * Function to get specified alias node from alias linked list
 */
char* getAliasNode(struct aliasNode* startANode, char *aliasMapData) {
    struct aliasNode* travANode = startANode;

    // Search the alias linked list for the specified alias node
    while (travANode != NULL) {
        if (strcmp(travANode->aliasMap, aliasMapData) == 0) {
            return travANode->aliasOrig;
        }
        travANode = travANode->nextANode;
    }
    return NULL;
}

/**
 * Function to print the entire alias linked list
 */
void printAliasList(struct aliasNode *startANode) {
    struct aliasNode *travANode = startANode;
    char buffer[MAXCMDSIZE];

    // Traverse the alias linked list to print each node
    while (travANode != NULL) {
        sprintf(buffer, "%s %s\n", travANode->aliasMap, travANode->aliasOrig);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        travANode = travANode->nextANode;
    }
}

/**
 * Function to print single specified alias node from linked list
 */
void printAliasNode(struct aliasNode *startANode, char *aliasMapData) {
    struct aliasNode *travANode = startANode;

    // Traverse the alias linked list for printing alias node
    while (travANode != NULL) {
        if (strcmp(travANode->aliasMap, aliasMapData) == 0) {
            printf("%s %s\n", travANode->aliasMap, travANode->aliasOrig);
            fflush(stdout);
            return;
        }
        travANode = travANode->nextANode;
    }
}

/**
 * Function to check if user command is a blank line
 */
int blankCmdCheck(char *userCmd) {
    int blank = 1, i = 0;

    // Check for blank spaces in user command
    for (i = 0; userCmd[i] != '\0'; i++) {
        if (!isspace(userCmd[i])) {
            blank = 0;
            break;
        }
    }
    return blank;
}

/**
 * Function to handle redirection of user command
 */
void redirection(char *userCmd) {
    int userIdx = 0, dupIdx = 0, redirIdx = 0, fd, len = strlen(userCmd);
    pid_t pid;
    int wstatus, redirFound = 0, valStdout, revertStdout = 0;
    char *dupCmd = (char*)malloc(MAXCMDSIZE*2*sizeof(char));
    char *redirCmd[MAXCMDSIZE];
    char buffer[MAXCMDSIZE];

    // Add spaces around redirection symbol for easy parsing
    for (userIdx = 0; userIdx < len; userIdx++) {
        if (userCmd[userIdx] == '>') {
            if (redirFound == 0) {
                dupCmd[dupIdx++] = ' ';
                dupCmd[dupIdx++] = userCmd[userIdx];
                dupCmd[dupIdx++] = ' ';
                redirFound = 1;
            } else {
                write(STDERR_FILENO, "Redirection misformatted.\n",
                                      strlen("Redirection misformatted.\n"));
                free(dupCmd);
                return;
            }
        } else {
            dupCmd[dupIdx++] = userCmd[userIdx];
        }
    }
    dupCmd[dupIdx++] = '\0';

    char *cmdStr = dupCmd + strlen(dupCmd) - 1;
    cmdStr--;
    *(cmdStr + 1) = '\0';

    // Checking user command misformats
    char *dupCmd1 = strdup(dupCmd);
    char *cmd1 = strtok(dupCmd1, " \t\r\n");
    int i = 0, j = 0;
    while (cmd1) {
        if (j == 1) {
            i++;
        }
        if (strcmp(cmd1, ">") == 0) {
            j = 1;
        }
        cmd1 = strtok(NULL, " \t\r\n");
    }

    if (j == 1 && i != 1) {
        write(STDERR_FILENO, "Redirection misformatted.\n",
                              strlen("Redirection misformatted.\n"));
        free(dupCmd1);
        free(dupCmd);
        return;
    }

    char* dupCmd2 = strdup(dupCmd);
    char* cmd = strtok(dupCmd2, " \t\r\n");
    if (strcmp(cmd, ">") == 0) {
        write(STDERR_FILENO, "Redirection misformatted.\n",
                              strlen("Redirection misformatted.\n"));
        free(dupCmd1);
        free(dupCmd2);
        free(dupCmd);
        return;
    }

    // Adding each token of user command to a redirection array
    while (cmd) {
        if (*cmd == '>') {
            char *file = strtok(NULL, " \t\r\n");
            valStdout = dup(1);
            revertStdout = 1;
            fd = open(file, O_TRUNC | O_CREAT | O_WRONLY, 0600);
            if (fd == -1) {
                sprintf(buffer, "Cannot write to file %s.\n", file);
                write(STDERR_FILENO, buffer, strlen(buffer));
                free(dupCmd1);
                free(dupCmd2);
                free(dupCmd);
                return;
            }
            dup2(fd, 1);
            close(fd);
        } else {
            redirCmd[redirIdx] = cmd;
            redirIdx++;
        }
        cmd = strtok(NULL, " \t\r\n");
    }
    redirCmd[redirIdx] = NULL;

    // Calling fork to create child process
    pid = fork();
    if (pid < 0) {
        write(STDERR_FILENO, "Failed to execute fork\n",
                              strlen("Failed to execute fork\n"));
        free(dupCmd1);
        free(dupCmd2);
        free(dupCmd);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        int ret = execv(redirCmd[0], redirCmd);
        if (ret == -1) {
            sprintf(buffer, "%s: Command not found.\n", redirCmd[0]);
            write(STDERR_FILENO, buffer, strlen(buffer));
            free(dupCmd1);
            free(dupCmd2);
            free(dupCmd);
            close(fd);
            _exit(123);
        }
    } else {
        waitpid(pid, &wstatus, 0);
    }

    // Restore file descriptor for stdout
    if (revertStdout) {
        dup2(valStdout, 1);
        close(valStdout);
    }

    // Free allocated memory for user command
    free(dupCmd1);
    free(dupCmd2);
    free(dupCmd);
}

/**
 * Main function to parse user command, 
 * check interactive or batch mode and
 * call redirection or alias functionality
 */
int main(int argc, char *argv[]) {
    FILE *inputFile = NULL;
    int defaultCase = 0, batch = 0;
    char buffer[MAXCMDSIZE];
    char *batchFile;

    // Checking if the shell is batch or interactive
    switch (argc) {
        case 1:
            inputFile = stdin;
            write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
            break;
        case 2:
            batch = 1;
            batchFile = strdup(argv[1]);
            inputFile = fopen(batchFile, "r");
            if (inputFile == NULL || inputFile < 0) {
                sprintf(buffer, "Error: Cannot open file %s.\n", batchFile);
                write(STDERR_FILENO, buffer, strlen(buffer));
                free(batchFile);
                exit(1);
            }
            break;
        default:
            write(STDERR_FILENO, "Usage: mysh [batch-file]\n",
                                  strlen("Usage: mysh [batch-file]\n"));
            defaultCase = 1;
            break;
    }

    if (defaultCase == 1) {
        free(batchFile);
        exit(1);
    }

    char *userCmd = (char*)malloc(MAXCMDSIZE*sizeof(char));
    char *constUserCmd = (char*)malloc(MAXCMDSIZE*sizeof(char));
    struct aliasNode* startANode = NULL;
    char *redirCmd[MAXCMDSIZE];
    int redirIdx = 0, upd = 0, aliasModified = 0, index;

    while (1) {
        if (fgets(userCmd, MAXCMDSIZE, inputFile) == NULL) {
            break;
        }

        // Print user command if batch mode
        if (batch) {
            write(STDOUT_FILENO, userCmd, strlen(userCmd));
        }

        // Blank user command check
        if (blankCmdCheck(userCmd)) {
            continue;
        }

        char* dupCmd = strdup(userCmd);
        char* cmd = strtok(dupCmd, " \t\r\n");

        // Checking for exit command
        if ((strcmp(cmd, "exit\n") == 0 || strcmp(cmd, "exit") == 0)) {
            free(dupCmd);
            break;
        }

        redirIdx = 0;
        while (cmd) {
            redirCmd[redirIdx++] = cmd;
            if (redirIdx < 2) {
                cmd = strtok(NULL, " \t\r\n");
            } else {
                cmd = strtok(NULL, "\n");
            }
        }

        redirCmd[redirIdx] = NULL;

        // Checking for variants of alias and unalias user command
        if ((strcmp(redirCmd[0], "alias") == 0) ||
            (strcmp(redirCmd[0], "alias\n") == 0)) {
            if (redirIdx == 1) {
                printAliasList(startANode);
            } else if (redirIdx == 2) {
                printAliasNode(startANode, redirCmd[1]);
            } else if (redirIdx >= 3) {
                if ((strcmp(redirCmd[1], "alias") == 0) ||
                    (strcmp(redirCmd[1], "unalias") == 0) ||
                    (strcmp(redirCmd[1], "exit") == 0)) {
                    write(STDERR_FILENO,
                            "alias: Too dangerous to alias that.\n",
                            strlen("alias: Too dangerous to alias that.\n"));
                } else {
                    upd = updateAliasNode(startANode, redirCmd[1], redirCmd[2]);
                    if (upd == 0) {
                        insertAliasNode(&startANode, redirCmd[1], redirCmd[2]);
                    }
                    upd = 0;
                }
            }
            if (!batch) {
                write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
            }
            free(dupCmd);
            continue;
        } else if ((strcmp(redirCmd[0], "unalias") == 0) ||
                   (strcmp(redirCmd[0], "unalias\n") == 0)) {
            if (redirIdx == 2) {
                deleteAliasNode(&startANode, redirCmd[1]);
            } else {
                write(STDERR_FILENO,
                        "unalias: Incorrect number of arguments.\n",
                        strlen("unalias: Incorrect number of arguments.\n"));
            }
            if (!batch) {
                write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
            }
            free(dupCmd);
            continue;
        }

        // Replace alias name for commad if present
        char *alias = getAliasNode(startANode, redirCmd[0]);
        aliasModified = 0;
        if (alias != NULL) {
            redirCmd[0] = alias;
            aliasModified = 1;
            strcpy(constUserCmd, redirCmd[0]);
            strcat(constUserCmd, " ");
            for (index = 1; index < redirIdx; index++) {
                strcat(constUserCmd, redirCmd[1]);
                strcat(constUserCmd, " ");
            }
        }

        if (aliasModified) {
            redirection(constUserCmd);
        } else {
            redirection(userCmd);
        }

        if (!batch) {
            write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
        }
        free(dupCmd);
    }

    // Free allocated memory
    deleteAliasList(&startANode);
    free(batchFile);
    free(constUserCmd);
    free(userCmd);
    fclose(inputFile);
}
