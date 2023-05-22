#include "message_slot.h"    

#include <stdio.h>
#include <string.h>
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *file_path;
    int channel_id, fd;
    char *message;
    size_t message_length, bytes_written;

    if (argc != 4) {
        perror("Invalid number of command lind arguments");
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]);
    message = argv[3];

    fd = open(file_path, O_RDWR);
    if( fd < 0 ) {
        perror("Can't open device file path: %s\n", file_path);
        exit(1);
    }

    // Set the channel id
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) != SUCCESS) {
        perror("Error setting channel id");
        close(fd);
        exit(1);
    }

    message_length = strlen(message);
    bytes_written = write(fd, message, message_length);
    if (bytes_written == -1) {
        perror("Error writing message");
        close(fd);
        exit(1);
    }

    close(fd);
    exit(0);
}