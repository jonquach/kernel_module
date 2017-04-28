#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

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

//Méthode appelée lors de l'ouverture du périphérique
int device_open(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "%s: opened device on inode %lu\n", DEVICE_NAME, inode->i_ino); 
  return 0;
}

//Méthode appelée lors de la fermeture du périphérique
int device_release(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "%s: released device on inode %lu\n", DEVICE_NAME, inode->i_ino);
  return 0;
}

//Méthode appelée lors d'une opération d'écriture sur le périphérique
ssize_t device_write(struct file* filp, const char* bufUserData, size_t count, loff_t* f_pos)
{
  int i;
  int j;
  int nb;
  char *tampon;

  //Verification que l'on peut écrire au moins un caractère
  if (virtual_device.size == DISK_SIZE)
    return -ENOSPC;

  i = 0;
  j = virtual_device.tail;
  nb = 0;
  //Allocation du tampon pour copy_from_user
  tampon = kmalloc(count * sizeof(char), GFP_KERNEL);
  if (copy_from_user(tampon, bufUserData, count) != 0)
    {
      return -EFAULT;
    }

  //Debut de la section critique
  down_interruptible(&virtual_device.sem);
  while (i < count)
    {
      //Copie du buffer dans le disque
      virtual_device.data[j] = tampon[i];
      i++;
      j++;
      nb++;
      //Fin du disque. Retour au debut (buffer circulaire)
      if (j > DISK_SIZE - 1)
	{
	  virtual_device.tail = 0;
	  j = 0;
	}
      //Plus de place sur le disque
      if ((virtual_device.size + nb)  == DISK_SIZE)
	break;
    }
  //Fin de la section critique
  up(&virtual_device.sem);
  
  virtual_device.tail = j;
  virtual_device.size += nb;
  (*f_pos) += nb;
  printk(KERN_ALERT "%s: head:%lu tail:%lu size:%lu\n", DEVICE_NAME, virtual_device.head, virtual_device.tail, virtual_device.size); 
  return nb;
}

//Méthode appelée lors d'une opération de lecture sur le périphérique
ssize_t device_read(struct file* filp, char* bufUserData, size_t count, loff_t* f_pos)
{

  int nb;
  int i;
  int j;
  char *kStr;
  
  nb = 0;
  i = 0;
  j = virtual_device.head;
  //Allocation du tampon pour copy_to_user
  kStr = kmalloc((count * sizeof(char)), GFP_KERNEL);
  //Debut de la zone critique
  down_interruptible(&virtual_device.sem);  
  while (i < count && i < virtual_device.size)
    {
      //Copie dans le tampon
      kStr[i] = virtual_device.data[j];
      virtual_device.data[j] = '\0';
      i++;
      j++;
      nb++;
      //Fin du disue retour au debut (buffer circulaire)
      if (j > DISK_SIZE - 1)
	{
	  virtual_device.head = 0;
	  j = 0;
	}
    }
  //Fin de la zone critique
  up(&virtual_device.sem);
  
  virtual_device.head = j;
  virtual_device.size -= nb;
  if (copy_to_user(bufUserData, kStr, nb) != 0)
    {
      return -EFAULT;
    }
  (*f_pos) += nb;
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

//méthode appelée lors de l'initialisation du pilote
static int driver_init(void) 
{
  int err;
  int i;

  //Routage des fonctions
  fops.open = device_open;
  fops.release = device_release;
  fops.write = device_write;
  fops.read = device_read;
  sema_init(&virtual_device.sem, 1);

  //Allocation du périphérique
  err = alloc_chrdev_region(&dev_num, FIRST_MINOR, NB_MINOR_ID, DEVICE_NAME);  
  
  if (err < 0) {
    printk(KERN_ALERT "%s: failed to obtain device numbers\n", DEVICE_NAME);
    return err;
  }

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

  //Recuperation du minor et major
  major_number = MAJOR(dev_num);
  minor_number = MINOR(dev_num);
  
  printk(KERN_ALERT "%s: module successfully loaded\n", DEVICE_NAME);
  printk(KERN_INFO "\tuse \"mknod /dev/%s c %d %d\" for device file", DEVICE_NAME, major_number, minor_number);
  return 0;
}

//Méthode appelée lors de la suppression du périphérique
static void driver_cleanup(void)
{
  unregister_chrdev_region(dev_num, NB_MINOR_ID);
  cdev_del(p_vircdev);

  printk(KERN_ALERT "Module %s successfully unloaded\n", DEVICE_NAME);
  
}

module_init(driver_init);
module_exit(driver_cleanup);
