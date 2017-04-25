#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>

struct cdev * p_vircdev;
dev_t dev_num;
int major_number;
int minor_number;

#define DEVICE_NAME "my_virtual_device"
#define FIRST_MINOR 0
#define NB_MINOR_ID 1

static int driver_init(void) 
{
	int err;

	err = alloc_chrdev_region(&dev_num, FIRST_MINOR, NB_MINOR_ID, DEVICE_NAME); 
	//register_chrdev(unsigned char major, const char *name, struct file_operations *fops);
	
	if (err < 0) {
		printk(KERN_ALERT "%s: failed to obtain device numbers\n", DEVICE_NAME);
		return err;
	}
	//major_number = major(dev_num);
	//minor_number = minor(dev_num);
	
	p_vircdev = cdev_alloc();
	p_vircdev->owner = DEVICE_NAME; //pas trop sur que ca soit DEVICE_NAME ici mais g pas d'autre idées
	cdev_add(&p_vircdev, dev_num, NB_MINOR_ID); 

	// ...

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
  cdev_del(&p_vircdev); 
  
  printk(KERN_ALERT "Module %s successfully unloaded\n", DEVICE_NAME);
}

module_init(driver_init);
module_exit(driver_cleanup);
