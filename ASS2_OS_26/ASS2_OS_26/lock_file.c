#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//#define FILE_NAME "file.txt"

int main(int argc , char* argv[]) {
    
 char path[100] ;
 strcpy(path,argv[1]);
  char temp[256]; 
  char *str; 
  str = strtok(path, "/");
  while (str != NULL) {
      strcpy(temp, str);
      //printf("%s\n", str);
      str = strtok(NULL, "/");
  }
    
    int fd = open(temp, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("fcntl");
        close(fd);
        return 1;
    }

    printf("File locked\n");
    while(1)
    {

    }
    close(fd);
    return 0;
}


    // sleep(100);

    // lock.l_type = F_UNLCK;
    // if (fcntl(fd, F_SETLK, &lock) == -1) {
    //     perror("fcntl");
    //     close(fd);
    //     return 1;
    // }

    // printf("File unlocked\n");

