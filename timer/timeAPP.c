#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/ioctl.h>

#define CLOSE_CMD       (_IO(0XEF, 0X1))
#define OPEN_CMD        (_IO(0XEF, 0X2))
#define SETPERIOD_CMD   (_IO(0XEF, 0X3))

int main(int argc, char *argv[]) {
    
    int fd, ret;
    char *filename;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];
    
    if (argc != 2) {

        printf("worong usage\r\n");
        return -1;

    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if (fd < 0) {

        printf("cant open file\r\n", filename);
        return -1;

    }
 
    while (1) {

        printf("Input CMD:");
        ret = scanf("%d", &cmd);
        if (ret != 1) {

            gets(str);

        }
        if (cmd == 1) cmd = CLOSE_CMD;

        if (cmd == 2) cmd = OPEN_CMD;

        if (cmd == 3) {

            cmd = SETPERIOD_CMD;
            printf("Input Timer Period:");
            ret = scanf("%d", &arg);
            
            if (ret != 1) {

                gets(str);

            }

        }
        ioctl(fd, cmd, arg);
    }

    return 0;
}