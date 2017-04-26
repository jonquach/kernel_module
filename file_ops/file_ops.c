#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "my_virtual_device"
#define FIRST_MINOR 0
#define NB_MINOR_ID 1
#define DISK_SIZE 1000

// The virtual device using RAM
struct CBuffDevice {
	char data[DISK_SIZE];
	unsigned long head;
	unsigned long tail;
	unsigned long size;
	struct semaphore sem;
} virtual_device;

// Character driver specific information
struct cdev * p_vircdev;
dev_t dev_num;
int major_number;
int minor_number;


int device_open(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "OPEN BITCH\n");
  return 0;
}

int device_release(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "RELEASE BITCH\n");
  return 0;
}

ssize_t device_write(struct file* filp, const char* bufUserData, size_t count, loff_t* f_pos)
{
  printk(KERN_ALERT "WRITE BITCH\n");
  return 0;
}

ssize_t device_read(struct file* filp, char* bufUserData, size_t count, loff_t* f_pos)
{
  printk(KERN_ALERT "READ BITCH\n");
  return 0;
}

// Specifying functions responding to file system calls
struct file_operations fops = {
  .owner = THIS_MODULE,  
  .open = NULL,  
  .release = NULL, 
  .write = NULL, 
  .read = NULL,  
};

static int driver_init(void) 
{
  int err;

  fops.open = device_open;
  fops.release = device_release;
  fops.write = device_write;
  fops.read = device_read;
  
  err = alloc_chrdev_region(&dev_num, FIRST_MINOR, NB_MINOR_ID, DEVICE_NAME);
  
  
  if (err < 0) {
    printk(KERN_ALERT "%s: failed to obtain device numbers\n", DEVICE_NAME);
    return err;
  }

  printk(KERN_ALERT "lol = %d\n", MAJOR(dev_num));

  p_vircdev = cdev_alloc();
  p_vircdev->owner = THIS_MODULE;
  p_vircdev->ops = &fops;
  err = cdev_add(p_vircdev, dev_num, NB_MINOR_ID);
    
  if (err < 0) {
    printk(KERN_ALERT "%s: unable to add cdev to kernel\n", DEVICE_NAME);
    return err;
  }
  printk(KERN_ALERT "%s: module successfully loaded\n", DEVICE_NAME);
  printk(KERN_INFO "\tuse \"mknod /dev/%s c %d %d\" for device file", DEVICE_NAME, major_number, minor_number);
  return 0;
}

static void driver_cleanup(void)
{
  unregister_chrdev_region(dev_num, NB_MINOR_ID);
  cdev_del(p_vircdev);

  printk(KERN_ALERT "Module %s successfully unloaded\n", DEVICE_NAME);
  
}

module_init(driver_init);
module_exit(driver_cleanup);
