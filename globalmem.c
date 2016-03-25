#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEMSIZE		0x1000
#define GLOBALMEMNAME		"globalmem"
#define GLOBALMEMDEVNUM		8

#define GLOBALMEMMAGIC		'g'
#define MEM_CLEAR		_IO(GLOBALMEMMAGIC, 0)

#define GM_PRT_INFO(fmt, args...) \
do { \
	printk(KERN_INFO "[%s](%s[%d]): " fmt, GLOBALMEMNAME, \
			__func__, __LINE__, ## args); \
} while (0)

#define GM_PRT_ERR(fmt, args...) \
do { \
	printk(KERN_ERR "[%s](%s[%d]): " fmt, GLOBALMEMNAME, \
			__func__, __LINE__, ## args); \
} while (0)

#define GM_PRT_DEBUG(fmt, args...) \
do { \
	printk(KERN_DEBUG "[%s](%s[%d]): " fmt, GLOBALMEMNAME, \
			__func__, __LINE__, ## args); \
} while (0)

static int globalmem_major;

struct gm_device {
	struct cdev cdev;
	unsigned mem[GLOBALMEMSIZE];
} *gm_dev;


static int gm_open(struct inode *ip, struct file *filp)
{
	struct gm_device *dev = container_of(ip->i_cdev,
			struct gm_device, cdev);

	filp->private_data = dev;
	GM_PRT_DEBUG("Device %d opened!\n", MINOR(dev->cdev.dev));
	return 0;
}

static int gm_release(struct inode *ip, struct file *filp)
{
	GM_PRT_DEBUG("Device %d released!\n", MINOR(ip->i_cdev->dev));
	return 0;
}

static ssize_t gm_read(struct file *filp, char __user *buf, size_t size,
		loff_t *posp)
{
	unsigned long p = *posp;
	unsigned int count = size;
	struct gm_device *dev = filp->private_data;
	int ret;

	if (p > GLOBALMEMSIZE)
		return 0;

	if (count > GLOBALMEMSIZE - p)
		count = GLOBALMEMSIZE - p;

	if(copy_to_user(buf, dev->mem + p, count)) {
		/*
		GM_PRT_ERR("Copy to user error!"
				"From: 0x%x To: 0x%x Cnt: %d\n",
				(unsigned)(dev->mem + p), (unsigned)buf,
				count);
		*/
		return -EFAULT;
	} else {
		*posp += count;
		ret = count;
		GM_PRT_INFO("Read %d bytes from %lu.\n", count, p);
	}

	return ret;
}

static ssize_t gm_write(struct file *filp, const char __user *buf, size_t size,
		loff_t *posp)
{
	unsigned long p = *posp;
	unsigned int count = size;
	struct gm_device *dev = filp->private_data;
	int ret = 0;

	if (p > GLOBALMEMSIZE)
		return 0;

	if (count > GLOBALMEMSIZE - p)
		count = GLOBALMEMSIZE - p;

	if (copy_from_user(dev->mem + p, buf, count) < 0) {
		/*
		GM_PRT_ERR("Copy from user error!"
				"From: 0x%x To: 0x%x Cnt: %d\n",
				(unsigned)(buf), (unsigned)(dev->mem + p),
				count);
		*/
		return -EFAULT;
	} else {
		*posp += count;
		ret = count;
		GM_PRT_INFO("Write %d bytes to %lu.\n", count, p);
	}

	return ret;
}

static long gm_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct gm_device *dev = filp->private_data;
	int ret = 0;

	switch (cmd) {
	case MEM_CLEAR:
		memset(dev->mem, 0, GLOBALMEMSIZE);
		break;
	default:
		GM_PRT_ERR("Unsupport command!\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

struct file_operations gm_fops = {
	.owner = THIS_MODULE,
	.read = gm_read,
	.write = gm_write,
	.open = gm_open,
	.release = gm_release,
	.unlocked_ioctl = gm_ioctl,
};

static int gm_setup_cdev(struct gm_device *dev, int index)
{
	int ret;
	dev_t devno = MKDEV(globalmem_major, index);

	cdev_init(&dev->cdev, &gm_fops);
	dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&dev->cdev, devno, 1);
	if (ret)
		GM_PRT_ERR("Adding cdev %d error!\n", index);

	return ret;
}

static int __init gm_init(void)
{
	dev_t devno;
	int ret, cnt;

	ret = alloc_chrdev_region(&devno, 0, GLOBALMEMDEVNUM, GLOBALMEMNAME);
	if (ret)
		return ret;
	globalmem_major = MAJOR(devno);

	gm_dev = kzalloc(sizeof(struct gm_device) * GLOBALMEMDEVNUM,
				GFP_KERNEL);
	if (!gm_dev){
		ret = -ENOMEM;
		goto fail0;
	}

	for (cnt = 0; cnt < GLOBALMEMDEVNUM; cnt++) {
		ret = gm_setup_cdev(gm_dev + cnt, cnt);
		if (ret)
			goto fail1;
	}
	GM_PRT_INFO("Install OK!\n");

	return 0;
fail1:
	kfree(gm_dev);
	gm_dev = NULL;
fail0:
	unregister_chrdev_region(devno, GLOBALMEMDEVNUM);
	return ret;
}

static void __exit gm_exit(void)
{
	int cnt;

	for (cnt = 0; cnt < GLOBALMEMDEVNUM; cnt++) {
		cdev_del(&gm_dev[cnt].cdev);
	}
	kfree(gm_dev);
	gm_dev = NULL;
	unregister_chrdev_region(MKDEV(globalmem_major, 0), GLOBALMEMDEVNUM);
	GM_PRT_INFO("Uninstall OK!\n");
}

module_init(gm_init);
module_exit(gm_exit);

MODULE_AUTHOR("WrittenBoi");
MODULE_DESCRIPTION("Global memory device driver for Study");
MODULE_LICENSE("GPL_V2");
