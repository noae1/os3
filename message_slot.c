// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

static struct chardev_info device_info;


static channel_node* find_channel(message_slot_file *slot_file, int channel_id) {
    struct channel_node *curr;

    curr = slot_file->head;
    while (curr) {
        if (curr->channel_id == channel_id){
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}


static int add_channel_to_slot (message_slot_file *slot_file, int channel_id_param) {
    
    channel_node *new_channel;

    new_channel = kmalloc(sizeof(channel_node), GFP_KERNEL);
    if (new_channel == NULL){
        return -ENOMEM;
    }

    new_channel->channel_id = channel_id_param;
    //new_channel->message = NULL;
    new_channel->message_length = 0;   
    new_channel->next = NULL;

    if (slot_file->head == NULL) {
        slot_file->head = new_channel;
        slot_file->tail = new_channel;
    } 
    else {
        slot_file->tail->next = new_channel;
        slot_file->tail = new_channel;
    }

    slot_file->number_channels++;

    return 0; // success
}

void free_channels(struct channel_node *head) {
    channel_node *current = head;
    channel_node *tmp;

    while (current != NULL) {
        tmp = current;
        current = current->next;
        kfree(tmp);
    }
}



//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode, struct file*  file )
{
  int minor_number = iminor(inode);

  printk("Invoking device_open(%p)\n", file);
  
  if (minor_number >= MAX_SLOTS_DEV_FILES){
      return -ENODEV;
  }

  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  // Free memory or perform cleanup if needed
  int minor_number = iminor(inode);
  //message_slot_file *slot = &device_info.slots[minor_number];

  printk("Invoking device_release(%p,%p)\n", inode, file);

  //free_channels(slot->head); // no need?
  file->private_data = NULL;

  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
  int i = 0;
  int minor_number = iminor(file->f_inode);
  //message_slot_file *slot = &device_info.slots[minor_number];
  ssize_t bytes_read = -1;
  channel_node* cur_channel;

   printk( "Invocing device_read(%p,%ld) - ",file, length);

  cur_channel = (channel_node*)file->private_data;
  // No channel has been set on the file descriptor
  if (cur_channel == NULL || buffer == NULL){
      return -EINVAL;
  }
  
  // No message exists on the channel
  if (cur_channel->message_length == 0){
      return -EWOULDBLOCK;
  }

  if (length < cur_channel->message_length){
      return -ENOSPC;  
  }

  /*
  // check the validity of user space buffers
  // access_ok returns true (nonzero) if the memory block may be valid
  if (access_ok(VERIFY_WRITE, &buffer, length) == 0){
      return -EINVAL;
  }
  // copy_to_user copy a block of data into user space.
  // Returns number of bytes that could not be copied.
  // On success, this will be zero.
  if (copy_to_user(buffer, cur_channel->message, cur_channel->message_length)){
      return -EFAULT;  // Error copying data to user space
  }
  */

  for (i = 0; i < cur_channel->message_length; i++){
      if (put_user(cur_channel->message[i], &buffer[i]) != 0){
          printk("Error copying data to user space using put_user\n");
          return -EFAULT;
      }
  }
  

  bytes_read = cur_channel->message_length;
  return bytes_read;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
  int i = 0;
  int minor_number = iminor(file->f_inode);
  //message_slot_file *slot = &device_info.slots[minor_number];
  ssize_t bytes_written = -1;
  channel_node* cur_channel;
  char cur_message[MAX_MESSAGE_SIZE];

  cur_channel = (channel_node*)file->private_data;

  // No channel has been set on the file descriptor
  if (cur_channel == NULL || buffer == NULL){
      return -EINVAL;
  }

  // If the passed message length is 0 or more than 128
  if (length == 0 || length > MAX_MESSAGE_SIZE){
      printk("The passed message length is 0 or more than 128");
      return -EMSGSIZE;
  }

  /*
  // check the validity of user space buffers
  // access_ok returns true (nonzero) if the memory block may be valid
  if (access_ok(VERIFY_READ, &buffer, length) == 0){
      return -EFAULT;
  }

  // copy_from_user returns number of bytes that could not be copied.
  // On success, this will be zero.
  // copy_from_user ensures atomicity and copies the entire buffer in a single operation.
  if (copy_from_user(cur_channel->message, buffer, length)) {
      return -EFAULT;
  }
  cur_channel->message_length = length;
  bytes_written = length;
  */

  
  // safely copy data from user space to kernel space:
  for (i = 0; i < length ; i++){
    if (get_user(cur_message[i], &buffer[i]) != 0){
      printk("Failed to copy data from user space");
      return -EFAULT;
    }
  }
  for (i = 0; i < length; i++) {
      cur_channel->message[i] = cur_message[i];
  }
  cur_channel->message_length = length;
  bytes_written = length;
  
  
  printk("Invoking device_write(%p,%ld) to channel : %d \n", file, length, cur_channel->channel_id);


  return bytes_written;
}


//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  int minor_number;
  message_slot_file* slot;
  channel_node* cur_channel;

  if (ioctl_param == 0 || MSG_SLOT_CHANNEL != ioctl_command_id){
      printk("Channel id must be nonZero and command should be MSG_SLOT_CHANNEL");
      return -EINVAL;
  }
  if (ioctl_param > UINT_MAX){
      printk("Channel id should be smaller then unsigned int max value\n");
      return -EINVAL;
  }

  
  minor_number = iminor(file->f_inode);
  printk( "Invoking ioctl: setting file descriptor's channel id "
          "channel id : %ld of minor : %d\n", ioctl_param,  minor_number);
  
  slot = &device_info.slots[minor_number];

  cur_channel = find_channel(slot, ioctl_param);

  if (cur_channel == NULL){ // ioctl_param channel is not found
    if (add_channel_to_slot(slot, ioctl_param) != 0){ // return 0 on success
      return -EINVAL;
    }
    // ioctl_param channel added to slot as the last element 
    // associate the passed channel id with the file descriptor it was invoked on
    file->private_data = slot->tail;
  }
  else{
    file->private_data = cur_channel;
  }
    
  

  return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
  .owner	        = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;
  int i = 0;
  // init dev struct
  memset( &device_info, 0, sizeof(struct chardev_info) );

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUMBER, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 ) {
    printk( KERN_ERR "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }

  for (i = 0;  i < MAX_SLOTS_DEV_FILES ; i++){
    device_info.slots[i].head = NULL;
  }

  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  // The module should free all memory that it allocated
  int i = 0;
  channel_node* messageSlot_head = NULL;

  for (i = 0; i < MAX_SLOTS_DEV_FILES ; i++){
    messageSlot_head = device_info.slots[i].head;
    if (messageSlot_head) {
        free_channels(messageSlot_head);
    }
  }

  unregister_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
