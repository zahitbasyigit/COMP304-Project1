/*
 * shelldon interface program

KUSIS ID: PARTNER NAME:
KUSIS ID: PARTNER NAME:

 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>


#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */
#define MAX_HISTORY_SIZE   10
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

int historySize = 0;

//FUNCTION DECLERATIONS

void initArguementSize(char *args[]);

int parseCommand(char inputBuffer[], char *args[], int *background);

char *concat(const char *s1, const char *s2);

int isLinuxCommand(char *command);

int arguementAtIndexEquals(char *args[], int index, char *string);

void addAndShiftElements(char *element[], int currentSize, int maxSize);

int main(void) {
    char inputBuffer[MAX_LINE];            /* buffer to hold the command entered */
    int background;                        /* equals 1 if a command is followed by '&' */
    char *args[ARGS_SIZE];            /* command line (of 80) has max of 40 arguments */
    pid_t child;                    /* process id of the child process */
    int status;                /* result from execv system call*/
    int shouldrun = 1;

    int i, upper;

    while (shouldrun) {                    /* Program terminates normally inside setup */
        background = 0;

        shouldrun = parseCommand(inputBuffer, args, &background);       /* get next command */

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

            addAndShiftElements(args, historySize, MAX_HISTORY_SIZE + 1);

            if (historySize < MAX_HISTORY_SIZE) {
                historySize++;
            }

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

                for (int y = 0; y < historySize; y++) {
                    printf("History %d:", y + 1);

                    for (int t = 0; t < ARGS_SIZE; t++) {
                        if (historyOfCommands[y][t] == NULL)
                            break;

                        printf("%s ", historyOfCommands[y][t]);
                    }

                    printf("\n");
                }
            }

            if (fork() == 0) {
                if (isLinuxCommand(inputBuffer)) {
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
                    printf("waiting for child to end\n");
                    wait(NULL);
                    printf("child has ended\n");
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
    if (index >= arguementSize)
        printf("Index Out of Bounds !!!!!!!!!!!!");

    if (strcmp(string, args[index]) == 0) {
        return 1;
    }

    return 0;
}

void addAndShiftElements(char *element[], int currentSize, int maxSize) {
    printf("init first element\n");
    printf("currentSize : %d, maxSize : %d\n", currentSize, maxSize);

    for (int i = currentSize; i >= 0; i--) {
        if (i == maxSize - 1) {
            continue;
        }

        //RESET HISTORY TO BE REPLACED
        for (int z = 0; z < ARGS_SIZE; z++) {
            historyOfCommands[i][z] = NULL;
        }

        if (i == 0) {
            for (int z = 0; z < ARGS_SIZE; z++) {
                if (element[z] == NULL)
                    break;

                historyOfCommands[i][z] = NULL;
                char *copied = malloc(strlen(element[z]) + 1);
                strcpy(copied, element[z]);
                historyOfCommands[i][z] = copied;

                printf("test1 : %s\n", historyOfCommands[i][z]);
                printf("test2 : %s\n", element[z]);
            }
        } else {
            for (int z = 0; z < ARGS_SIZE; z++) {
                if (historyOfCommands[i - 1][z] == NULL)
                    break;

                char *copied = malloc(strlen(historyOfCommands[i - 1][z]) + 1);
                strcpy(copied, historyOfCommands[i - 1][z]);
                historyOfCommands[i][z] = copied;
            }
        }
    }
}

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}