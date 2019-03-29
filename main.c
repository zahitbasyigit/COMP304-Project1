/*
 * shelldon interface program

KUSIS ID: PARTNER NAME:
KUSIS ID: PARTNER NAME:

 */

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>


#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */
#define MAX_HISTORY_PRINT_COUNT 10
#define MAX_HISTORY_SIZE   500
#define ARGS_SIZE MAX_LINE / 2 + 1

int DEBUG_MODE = 1;

const char *myCommands[] = {
        "test",
        "renk",
        "dance",
};

int arguementSize = 0;


//HISTORY
char *commandStr;

char *historyOfCommands[MAX_HISTORY_SIZE][ARGS_SIZE];

int historyCount = 0;

//FUNCTION DECLERATIONS

void initArguementSize(char *args[]);

int parseCommand(char inputBuffer[], char *args[], int *background);

int parseHistoryFromIndexCommand(char *args[]);

char *concat(const char *s1, const char *s2);

int isLinuxCommand(char *command);

int arguementAtIndexEquals(char *args[], int index, char *string);

void addElementToHistory(char **element);

void printHistory();

//CODE SEARCH
void printWordOccurancesInFile(char *filename, char *word);

void codeSearch(char *args[]);

void birdakika(char *args[]);

void recursiveCodeSearch(char *query, const char *name, int indent);

int main(void) {
    char inputBuffer[MAX_LINE];            /* buffer to hold the command entered */
    int background;                        /* equals 1 if a command is followed by '&' */
    char *args[ARGS_SIZE];            /* command line (of 80) has max of 40 arguments */
    pid_t child;                    /* process id of the child process */
    int status;                /* result from execv system call*/
    int shouldrun = 1;
    int shouldreadfromuser = 1;

    int i, upper;

    while (shouldrun) {                    /* Program terminates normally inside setup */
        background = 0;

        if (shouldreadfromuser) {
            shouldrun = parseCommand(inputBuffer, args, &background);       /* get next command */
        } else {
            shouldreadfromuser = 1;
        }

        if (strncmp(inputBuffer, "exit", 4) == 0)
            shouldrun = 0;     /* Exiting from shelldon*/

        if (shouldrun) {
            /*
          After reading user input, the steps are
          (1) Fork a child process using fork()
          (2) the child process will invoke execv()
          (3) if command included &, parent will invoke wait()
             */

            initArguementSize(args);

            //Print args for debugging
            if (DEBUG_MODE) {
                /*
                printf("Input Buffer : %s\n", inputBuffer);

                for (int t = 0; t < ARGS_SIZE; t++) {
                    if (args[t] == NULL)
                        break;

                    printf("Arguments %d: %s\n", t, args[t]);
                }

                 */

            }

            int commandHistoryRunId = parseHistoryFromIndexCommand(args);

            if (strcmp("!!", inputBuffer) == 0) {
                shouldreadfromuser = 0;
                strcpy(inputBuffer, historyOfCommands[historyCount - 1][0]);

                for (int j = 0; j < ARGS_SIZE; j++) {
                    if (historyOfCommands[historyCount - 1][j] == NULL)
                        break;

                    char *copied = malloc(strlen(historyOfCommands[historyCount - 1][j]) + 1);
                    strcpy(copied, historyOfCommands[historyCount - 1][j]);
                    args[j] = copied;
                }

                continue;

            } else if (commandHistoryRunId != -1) {
                shouldreadfromuser = 0;
                strcpy(inputBuffer, historyOfCommands[commandHistoryRunId - 1][0]);

                for (int j = 0; j < ARGS_SIZE; j++) {
                    if (historyOfCommands[commandHistoryRunId - 1][j] == NULL)
                        break;

                    char *copied = malloc(strlen(historyOfCommands[commandHistoryRunId - 1][j]) + 1);
                    strcpy(copied, historyOfCommands[commandHistoryRunId - 1][j]);
                    args[j] = copied;
                }

                continue;
            }

            addElementToHistory(args);

            if (fork() == 0) {
                if (strcmp("history", inputBuffer) == 0) {
                    printHistory();
                } else if (strcmp("codesearch", inputBuffer) == 0) {
                    codeSearch(args);
                } else if(strcmp("birdakika", inputBuffer) == 0){
                	birdakika(args);

                } else if (isLinuxCommand(inputBuffer)) {
                    int append = arguementAtIndexEquals(args, arguementSize - 2, ">>");
                    //printf("Append: %d\n", append);
                    int truncate = arguementAtIndexEquals(args, arguementSize - 2, ">");
                    //printf("Truncate: %d\n", truncate);

                    if (append || truncate) {
                        const size_t filenameSize = strlen(args[arguementSize - 1]) + 1;
                        char *copiedFilename = malloc(filenameSize);
                        strncpy(copiedFilename, args[arguementSize - 1], filenameSize);

                        args[arguementSize - 2] = NULL;
                        args[arguementSize - 1] = NULL;

                        int returnVal;

                        if (truncate) {
                            returnVal = open(copiedFilename, O_CREAT | O_RDWR | O_TRUNC, 0777);
                        } else {
                            returnVal = open(copiedFilename, O_CREAT | O_RDWR | O_APPEND, 0777);
                        }

                        close(1);
                        dup2(returnVal, 1);
                        close(returnVal);
                    }

                    char *execFile = concat("/bin/", inputBuffer);
                    status = execv(execFile, args);
                    //printf("execv status: %d\n", status);
                    free(execFile);
                } else {
                    printf("not implemented\n");
                }
            } else {
                if (background == 0) {
                    //printf("waiting for child to end\n");
                    wait(NULL);
                    //printf("child has ended\n");
                }
            }

        }
    }
    return 0;
}

/** 
 * The parseCommand function below will not return any value, but it will just: read
 * in the next command line; separate it into distinct arguments (using blanks as
 * delimiters), and set the args array entries to point to the beginning of what
 * will become null-terminated, C-style strings. 
 */

int parseCommand(char inputBuffer[], char *args[], int *background) {
    int length,        /* # of characters in the command line */
            i,        /* loop index for accessing inputBuffer array */
            start,        /* index where beginning of next command parameter is */
            ct,            /* index of where to place the next parameter into args[] */
            command_number;    /* index of requested command number */

    ct = 0;

    /* read what the user enters on the command line */
    do {
        printf("shelldon>");
        fflush(stdout);
        length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
    } while (inputBuffer[0] == '\n'); /* swallow newline characters */

    /**
     *  0 is the system predefined file descriptor for stdin (standard input),
     *  which is the user's screen in this case. inputBuffer by itself is the
     *  same as &inputBuffer[0], i.e. the starting address of where to store
     *  the command that is read, and length holds the number of characters
     *  read in. inputBuffer is not a null terminated C-string. 
     */
    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

    /** 
     * the <control><d> signal interrupted the read system call 
     * if the process is in the read() system call, read returns -1
     * However, if this occurs, errno is set to EINTR. We can check this  value
     * and disregard the -1 value 
     */

    if ((length < 0) && (errno != EINTR)) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /**
     * Parse the contents of inputBuffer
     */

    for (i = 0; i < length; i++) {
        /* examine every character in the inputBuffer */

        switch (inputBuffer[i]) {
            case ' ':
            case '\t' :               /* argument separators */
                if (start != -1) {
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&') {
                    *background = 1;
                    inputBuffer[i - 1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */

    /**
     * If we get &, don't enter it in the args array
     */

    if (*background)
        args[--ct] = NULL;

    args[ct] = NULL; /* just in case the input line was > 80 */

    return 1;

} /* end of parseCommand routine */

void initArguementSize(char *args[]) {
    int n = 0;

    for (int i = 0; i < ARGS_SIZE; i++) {
        if (args[i] == NULL) {
            break;
        }

        n++;
    }

    arguementSize = n;
}

int isLinuxCommand(char *command) {
    int arrSize = (sizeof(myCommands) / sizeof(const char *));

    for (int i = 0; i < arrSize; i++) {
        if (strcmp(command, myCommands[i]) == 0) {
            return 0;
        }
    }

    return 1;
}

int arguementAtIndexEquals(char *args[], int index, char *string) {
    if (index >= arguementSize) {
        //index of out bounds --> not equals
        return 0;
    }

    if (strcmp(string, args[index]) == 0) {
        return 1;
    }

    return 0;
}

void addElementToHistory(char **element) {
    if (strcmp(element[0], "history") == 0)
        return;

    for (int i = 0; i < ARGS_SIZE; i++) {
        if (element[i] == NULL)
            break;

        char *copied = malloc(strlen(element[i]) + 1);
        strcpy(copied, element[i]);
        historyOfCommands[historyCount][i] = copied;
    }

    historyCount++;
}

void printHistory() {
    int printCount = 0;

    for (int y = historyCount - 1; y >= 0; y--) {
        int displayId = y + 1;
        printCount++;

        if (printCount > MAX_HISTORY_PRINT_COUNT)
            break;

        if (historyCount > MAX_HISTORY_SIZE) {
            displayId = historyCount - MAX_HISTORY_SIZE + displayId;
        }

        printf("%d ", displayId);

        for (int t = 0; t < ARGS_SIZE; t++) {
            if (historyOfCommands[y][t] == NULL)
                break;

            printf("%s ", historyOfCommands[y][t]);
        }

        printf("\n");
    }
}

int parseHistoryFromIndexCommand(char *args[]) {
    char *first = args[0];
    char *rest = malloc(strlen(first));

    int numberCount = 0;

    if (first[0] == '!') {
        while (1) {
            numberCount++;
            if (first[numberCount] == '\0') {
                int historyId = atoi(rest);
                if (historyId > historyCount) {
                    printf("Invalid history id\n");
                    return -1;
                }

                return historyId;
            }

            rest[numberCount - 1] = first[numberCount];
        }
    } else {
        return -1;
    }
}

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void printWordOccurancesInFile(char *filename, char *word) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int linecount = 1;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Unable to open file please try again\n");
        return;
    }

    while ((read = getline(&line, &len, fp)) != -1) {

        //check if word is contained in line
        if (strstr(line, word) != NULL) {
            printf("%d: %s -> %s", linecount, filename, line);
        }

        linecount++;
    }

    fclose(fp);
    if (line)
        free(line);
}

void birdakika(char *args[]){
    if(args[1]==NULL) printf("Missing parameters.\n");
    else if(args[2]== NULL) printf("Folder name must be given.\n");
    else{
        char *timeGiven = args[1];
        char *fileName = args[2];
        char *hour;
        char *min;    
   
        hour = strtok(timeGiven , ".");
        min = strtok(NULL , ".");
        
        
        ////This job starts the music
        char startCommand[300] = "(crontab -u $USER -l ; echo \""; 
        strcat(startCommand, min);
        strcat(startCommand, " ");
        strcat(startCommand, hour);
        strcat(startCommand, " * * * mpg321 ");
        strcat(startCommand, fileName);
        strcat(startCommand, "\") | crontab -u $USER -");                
        
        //// This job stops the job
        int a = atoi(min) + 1;
        char stopMin[100];
        sprintf(stopMin, "%d", a);
        char stopCommand[300] = "(crontab -u $USER -l ; echo \""; 
        strcat(stopCommand, stopMin);
        strcat(stopCommand, " ");
        strcat(stopCommand, hour);
        strcat(stopCommand, " * * * pkill mpg321 ");
        strcat(stopCommand, "\") | crontab -u $USER -");

        system(startCommand);
        system(stopCommand);
    }
}

void codeSearch(char *args[]) {
    if (args[1] != NULL) {

        int isRecursive = 0;
        int isTargeted = 0;

        if (arguementAtIndexEquals(args, 1, "-r")) {
            isRecursive = 1;
        } else if (arguementAtIndexEquals(args, 2, "-f")) {
            isTargeted = 1;
        }

        char *arg;

        if (isRecursive) {
            arg = args[2];
        } else {
            arg = args[1];
        }

        char *query = malloc(strlen(arg));

        if (arg[0] != '"') {
            printf("Invalid query enter with \"\"\n");
            return;
        }

        int argcnt = 1;
        int querycnt = 0;

        while (1) {
            if (arg[argcnt] == '"') {
                break;
            } else {
                query[querycnt] = arg[argcnt];
                querycnt++;
            }

            argcnt++;
        }

        query[querycnt] = '\0';

        printf("expected query : %s\n", query);

        if (isTargeted) {
            if (args[2] != NULL) {
                printWordOccurancesInFile(args[3], query);
            }
        } else if (isRecursive) {
            recursiveCodeSearch(query, ".", 0);
        } else {
            DIR *d;
            struct dirent *dir;
            d = opendir(".");
            if (d) {
                while ((dir = readdir(d)) != NULL) {
                    printWordOccurancesInFile(dir->d_name, query);
                }
                closedir(d);
            }
        }
    } else {
        printf("Invalid arguments for codesearch\n");
    }
}


void recursiveCodeSearch(char *query, const char *name, int indent) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            recursiveCodeSearch(query, path, indent + 2);
        } else {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            printWordOccurancesInFile(path, query);
        }
    }

    closedir(dir);
}