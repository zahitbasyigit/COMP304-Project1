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


int main(void) {
    printf("sleeping starts\n");
    sleep(5);
    printf("sleeping ended\n");

    return 0;
}