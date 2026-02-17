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

DEFINE_MUTEX(ipc_mutex);
DECLARE_WAIT_QUEUE_HEAD(ipc_wq);

static bool isExit=false;

static int ipc_open(struct inode *in, struct file *fp){
	KLOG("open");
	return 0;
}

static ssize_t ipc_read(struct file *fp, char __user *buf, size_t len, loff_t *offset){

	if (wait_event_interruptible(ipc_wq, msg_len || isExit)){
		return -ERESTARTSYS;
	}

	if (!msg_len) { 

		KLOG("read - !msg_len");
		return -EFAULT; 
	}

	if (len < (msg_len)){ 
		KLOG("read - small buffer");
		return -EFAULT; 
	}

	mutex_lock(&ipc_mutex);
	if (copy_to_user(buf, msg, msg_len)){
		goto err_mutex_unlock_rd;
	}

	msg[0] = '\0';
	int tmp_len = msg_len;
	msg_len=0;

	wake_up_interruptible(&ipc_wq);
	mutex_unlock(&ipc_mutex);
	return tmp_len;

err_mutex_unlock_rd:
	mutex_unlock(&ipc_mutex);
	return -EFAULT;
}

static ssize_t ipc_write(struct file *fp, const char __user *buf, size_t len, loff_t *offset){
	mutex_lock(&ipc_mutex);
	if (msg_len){
		KLOG("write - msg_len>0");
		goto err_mutex_unlock_wr;
	}

	if (len > (BUF_SZ-1)){
		KLOG(" Overflow");
		goto err_mutex_unlock_wr;
	}

	if (copy_from_user(msg, buf, len)){
		KLOG(" copy-failed");
		goto err_mutex_unlock_wr;
	}
	msg[len] = '\0';
	msg_len = len;

	wake_up_interruptible(&ipc_wq);
	mutex_unlock(&ipc_mutex);

	return len;

err_mutex_unlock_wr:
	mutex_unlock(&ipc_mutex);
	return -EFAULT;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = ipc_open,
	.read = ipc_read,
	.write = ipc_write
};

static int __init ipc_init(void){
	int ret;

	major = register_chrdev(0, IPC_DEV, &fops);
	if (major < 0){
		return major;
	}

	ipc_class = class_create(IPC_CLASS);
	if (IS_ERR(ipc_class)) {
		ret = PTR_ERR(ipc_class);
		goto err_chrdev;
	}

	ipc_dev = device_create(ipc_class, NULL, MKDEV(major, 0), NULL, IPC_DEV);
	if (IS_ERR(ipc_dev)){
		ret = PTR_ERR(ipc_dev);
		goto err_class;
	}

	KLOG("INIT");
	return 0;

err_class:
    class_destroy(ipc_class);

err_chrdev:
    unregister_chrdev(major, IPC_DEV);
    return ret;
}

static void __exit ipc_exit(void){
	isExit = true;
	wake_up_all(&ipc_wq);
	
	if (ipc_class) {
		if (major){
			device_destroy(ipc_class, MKDEV(major, 0));
		}
	}

	if (ipc_class) {
		class_destroy(ipc_class);
	}
	
	if (major > 0){
		unregister_chrdev(major, IPC_DEV);
	}
	KLOG("EXIT");
}

module_init(ipc_init);
module_exit(ipc_exit);
