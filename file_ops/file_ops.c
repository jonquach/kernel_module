#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>


#define DEVICE_NAME "my_virtual_device"
#define FIRST_MINOR 0
#define NB_MINOR_ID 1
#define DISK_SIZE 1024*1024

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
  printk(KERN_ALERT "%s: opened device on inode %lu\n", DEVICE_NAME, inode->i_ino); 
  printk(KERN_ALERT "OPEN BITCH\n");
  return 0;
}

int device_release(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "%s: released device on inode %lu\n", DEVICE_NAME, inode->i_ino);
  printk(KERN_ALERT "RELEASE BITCH\n");
  return 0;
}

ssize_t device_write(struct file* filp, const char* bufUserData, size_t count, loff_t* f_pos)
{
  int i;
  int j;
  int nb;
  char *tampon;

  i = 0;
  j = virtual_device.tail;
  nb = 0;
  tampon = kmalloc(count * sizeof(char), GFP_KERNEL);  
  copy_from_user(tampon, bufUserData, count);
  
  while (i < count)
    {
      virtual_device.data[j] = tampon[i];
      i++;
      j++;
      nb++;
    }
  virtual_device.tail = j;
  virtual_device.size += nb;

  printk(KERN_ALERT "%s: head:%lu tail:%lu size:%lu\n", DEVICE_NAME, virtual_device.head, virtual_device.tail, virtual_device.size);  
  return nb;
}

ssize_t device_read(struct file* filp, char* bufUserData, size_t count, loff_t* f_pos)
{

  int nb;
  int i;
  int j;
  char *kStr;

  printk(KERN_ALERT "1111 %d\n", count);
  nb = 0;
  i = 0;
  j =  virtual_device.head;
  kStr = kmalloc((count * sizeof(char) + 1), GFP_KERNEL);
  printk(KERN_ALERT "READ BITCH\n");
  printk(KERN_ALERT "2222\n");  
  while (i < count && i < virtual_device.size)
    {
      printk(KERN_ALERT "33333\n");  
      kStr[i] = virtual_device.data[j];
      printk(KERN_ALERT "---> %c\n", virtual_device.data[j]);  
      virtual_device.data[j] = '\0';
      i++;
      j++;
      nb++;
    }
  printk(KERN_ALERT "4444\n");  
  kStr[i] = '\0';
  printk(KERN_ALERT "plop = %s\n", kStr);  
  
  virtual_device.head += nb;
  virtual_device.size -= nb;
  printk(KERN_ALERT "5555\n");  
  copy_to_user(bufUserData, kStr, nb);
  printk(KERN_ALERT "6666\n");  
  return nb;
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
  int i;

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

  //init le buffer circulaire
  i = 0;
  while (i < DISK_SIZE)
    {
      virtual_device.data[i] = '\0';
      i++;
    }
  virtual_device.head = 0;
  virtual_device.tail = 0;
  virtual_device.size = 0;
  
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
