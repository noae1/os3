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
    size_t message_length, bytes_read;
    char buffer[MAX_MESSAGE_SIZE]; // 128
    ssize_t bytes_written_to_output;

    if (argc != 3) {
        perror("Invalid number of command lind arguments");
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]);

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

    bytes_read = read(fd, buffer, MAX_MESSAGE_SIZE);
    if (bytes_read <= 0) {
        perror("Error reading message");
        close(fd);
        exit(1);
    }

    close(fd);

    bytes_written_to_output = write(STDOUT_FILENO, buffer, bytes_read);
    if (bytes_written_to_output != bytes_read) {
        perror("Error writing message to stdout");
        exit(1);
    }

    exit(0);






}