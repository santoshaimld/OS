#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>

#include <linux/fs.h>
#include <linux/wait.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Santosh");
MODULE_DESCRIPTION("Simple IPC");

#define BUF_SZ 1024
#define IPC_DEV "ipc_dev"
#define IPC_CLASS "ipc_class"

static int major;
static struct class *ipc_class = NULL;
static struct device *ipc_dev = NULL;

static char msg[BUF_SZ];
static int msg_len=0;
#define KLOG(str) {printk(KERN_INFO "%s:%d - %s", __func__, __LINE__, str);}

DECLARE_WAIT_QUEUE_HEAD(ipc_wq);

static bool isExit=false;

static int ipc_open(struct inode *in, struct file *fp){
	KLOG("open");
	return 0;
}

static ssize_t ipc_read(struct file *fp, char __user *buf, size_t len, loff_t *offset){

#if 0
	while (wait_event_interruptible(ipc_wq, msg_len || isExit));
#else
	if (wait_event_interruptible(ipc_wq, msg_len || isExit)){
		return -ERESTARTSYS;
	}
#endif
	if (!msg_len) { 

		KLOG("read - !msg_len");
		return -EFAULT; 
	}

	if (len < (msg_len)){ 
		KLOG("read - small buffer");
		return -EFAULT; 
	}

	if (copy_to_user(buf, msg, msg_len)){return -EFAULT;}

	KLOG("read - success");

	msg[0] = '\0';
	int tmp_len = msg_len;
	msg_len=0;

	wake_up_interruptible(&ipc_wq);
	return tmp_len;
}

static ssize_t ipc_write(struct file *fp, const char __user *buf, size_t len, loff_t *offset){

	if (msg_len){
		KLOG("write - msg_len>0");
		return -EFAULT;
	}

	if (len > (BUF_SZ-1)){
		KLOG(" Overflow");
		return -EFAULT;
	}

	if (copy_from_user(msg, buf, len)){
		KLOG(" copy-failed");
		return -EFAULT;
	}
	msg[len] = '\0';
	msg_len = len;
	printk(KERN_INFO " ipc_write: len: %ld", len);

	wake_up_interruptible(&ipc_wq);
	return len;
}

static struct file_operations fops = {
	.open = ipc_open,
	.read = ipc_read,
	.write = ipc_write
};

static int __init ipc_init(void){
	major = register_chrdev(0, IPC_DEV, &fops);

	ipc_class = class_create(IPC_CLASS);
	ipc_dev = device_create(ipc_class, NULL, MKDEV(major, 0), NULL, IPC_DEV);

	KLOG("INIT");
	return 0;
}

static void __exit ipc_exit(void){
	isExit = true;
	wake_up_all(&ipc_wq);
	device_destroy(ipc_class, MKDEV(major, 0));
	class_unregister(ipc_class);
	class_destroy(ipc_class);
	
	unregister_chrdev(major, IPC_DEV);
	KLOG("EXIT");
}

module_init(ipc_init);
module_exit(ipc_exit);

