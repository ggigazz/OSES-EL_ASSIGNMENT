#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "data.h"

static dev_t heartbeatmod_dev;

struct cdev heartbeatmod_cdev;

struct class *myclass = NULL;

static char buffer[64];

static int i = 0;

ssize_t heartbeatmod_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
	i%=2048;
	int data = ppg[i++];
//	READ_DATA_FROM_THE_HW(&data);
//	printk(KERN_INFO "[heartbeatmod]read (count=%d, offset=%d)\n", (int)count, (int)*f_pos );
	copy_to_user(buf,&data,sizeof(int));
	return count;
}

struct file_operations heartbeatmod_fops = {
	.owner = THIS_MODULE,
	.read = heartbeatmod_read,
};

static int __init heartbeatmod_module_init(void){
	printk(KERN_INFO "Loading heartbeatmod\n");

	alloc_chrdev_region(&heartbeatmod_dev, 0, 1, "heartbeatmod");
	printk(KERN_INFO "%s\n", format_dev_t(buffer, heartbeatmod_dev));

	myclass = class_create(THIS_MODULE, "heartbeatmod_sys");
	device_create(myclass, NULL, heartbeatmod_dev, NULL, "heartbeatmod");

	cdev_init(&heartbeatmod_cdev, &heartbeatmod_fops);
	heartbeatmod_cdev.owner = THIS_MODULE;
	cdev_add(&heartbeatmod_cdev, heartbeatmod_dev, 1);

	return 0;
}

static void __exit heartbeatmod_module_cleanup(void){
	printk(KERN_INFO "Cleaning-up heartbeatmod_dev.\n");

	cdev_del(&heartbeatmod_cdev);
	unregister_chrdev_region(heartbeatmod_dev, 1);
}

module_init(heartbeatmod_module_init);
module_exit(heartbeatmod_module_cleanup);

MODULE_AUTHOR("Andrea Cencio");
MODULE_LICENSE("GPL");
