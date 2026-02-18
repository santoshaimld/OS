#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>

#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Santosh");
MODULE_DESCRIPTION("Simple IPC");

#define BUF_SZ 256
#define IPC_DEV "ipc_dev"
#define IPC_CLASS "ipc_class"

static int major;
static struct class *ipc_class = NULL;
static struct device *ipc_dev = NULL;

static char msg[BUF_SZ];
static int msg_len=0;
#define KLOG(str) {printk(KERN_INFO "%s:%d - %s", __func__, __LINE__, str);}

DEFINE_MUTEX(ipc_mutex);
DECLARE_WAIT_QUEUE_HEAD(ipc_read_wq);
DECLARE_WAIT_QUEUE_HEAD(ipc_write_wq);

static bool isExit=false;

static int ipc_open(struct inode *in, struct file *fp){
	KLOG("open");
	return 0;
}

static ssize_t ipc_read(struct file *fp, char __user *buf, size_t len, loff_t *offset){
	int err_code=0;
	char local_msg[BUF_SZ];
	int tmp_len = 0;

	if (wait_event_interruptible(ipc_read_wq, 
					READ_ONCE(msg_len) || READ_ONCE(isExit))){
		return -ERESTARTSYS;
	}

	mutex_lock(&ipc_mutex);
	if (isExit){
		err_code = -ENODEV;
		goto err_mutex_unlock_rd;
	}
	if (!msg_len) { 
		KLOG("read - !msg_len");
		err_code = -EAGAIN;
		goto err_mutex_unlock_rd;
	}

	if (len < (msg_len)){ 
		KLOG("read - small buffer");
		err_code = -EINVAL;
		goto err_mutex_unlock_rd;
	}

	memcpy(local_msg, msg, msg_len);
	msg[0] = '\0';
	tmp_len = msg_len;
	msg_len=0;
	mutex_unlock(&ipc_mutex);

	if (copy_to_user(buf, local_msg, tmp_len)){
		return -EFAULT;
	}

	wake_up_interruptible(&ipc_write_wq);
	return tmp_len;

err_mutex_unlock_rd:
	mutex_unlock(&ipc_mutex);
	return err_code;
}

static ssize_t ipc_write(struct file *fp, const char __user *buf, size_t len, loff_t *offset){
	int err_code = 0;
	char tmp_buf[BUF_SZ];

	if (len > (BUF_SZ-1)){
		KLOG(" Overflow");
		return -EINVAL;
	}

	if (copy_from_user(tmp_buf, buf, len)){
		KLOG(" copy-failed");
		return -EFAULT;
	}

	if (wait_event_interruptible(ipc_write_wq, !READ_ONCE(msg_len) || READ_ONCE(isExit))){
		return -ERESTARTSYS;
	}

	mutex_lock(&ipc_mutex);
	if (isExit){
		err_code = -ENODEV;
		goto err_mutex_unlock_wr;
	}

	if (msg_len){
		err_code = -EBUSY;
		KLOG("write - msg_len>0");
		goto err_mutex_unlock_wr;
	}

	memcpy(msg, tmp_buf, len);
	msg[len] = '\0';
	msg_len = len;
	mutex_unlock(&ipc_mutex);

	wake_up_interruptible(&ipc_read_wq);

	return len;

err_mutex_unlock_wr:
	mutex_unlock(&ipc_mutex);
	return err_code;
}

static __poll_t ipc_poll(struct file *file, struct poll_table_struct *wait)
{
	__poll_t mask = 0;

	poll_wait(file, &ipc_read_wq, wait);
	poll_wait(file, &ipc_write_wq, wait);

	mutex_lock(&ipc_mutex);
	if (isExit) {
		mask |= POLLHUP;
		goto out;
	}

	if (msg_len > 0){
		mask |= POLLIN | POLLRDNORM;
	}

	if (!msg_len){
		mask |= POLLOUT | POLLWRNORM;
	}

out:
	mutex_unlock(&ipc_mutex);
	return mask;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = ipc_open,
	.read = ipc_read,
	.write = ipc_write,
	.poll = ipc_poll
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
	mutex_lock(&ipc_mutex);
	isExit = true;
	mutex_unlock(&ipc_mutex);

	wake_up_all(&ipc_read_wq);
	wake_up_all(&ipc_write_wq);
	
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
