#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUMBER 235
#define MAX_SLOTS_DEV_FILES 256  //-
#define MAX_MESSAGE_SIZE 128  

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned long) // IOR IOW

#define SUCCESS 0
#define DEVICE_RANGE_NAME "char_dev"
#define DEVICE_FILE_NAME "simple_char_dev"


typedef struct channel_node {
    int channel_id;  //long
    char message[MAX_MESSAGE_SIZE]; // 128
    int message_length;
    struct channel_node *next;
}channel_node;

// Data structure to store message slots
typedef struct message_slot_file {
    struct channel_node *head;
    int number_channels;
    struct channel_node *tail;

    
}message_slot_file;

struct chardev_info {
    struct message_slot_file slots[MAX_SLOTS_DEV_FILES];  // 256 or 257?
};

#endif
