#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define GPIOLED_CNT			1		  	
#define GPIOLED_NAME		"gpioled"	
#define LEDOFF 				0			
#define LEDON 				1			


struct gpioled_dev{
	dev_t devid;			
	struct cdev cdev;		
	struct class *class;	
	struct device *device;	
	int major;				
	int minor;				
	struct device_node	*nd; 
	int led_gpio;			
	struct mutex lock;		/* 互斥体 */
};

struct gpioled_dev gpioled;	


static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled; /* 设置私有数据 */

	/* 获取互斥体,可以被信号打断 */
	if (mutex_lock_interruptible(&gpioled.lock)) {
		return -ERESTARTSYS;
	}
#if 0
	mutex_lock(&gpioled.lock);	/* 不能被信号打断 */
#endif

	return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}


static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;
	struct gpioled_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];		/* 获取状态值 */

	if(ledstat == LEDON) {	
		gpio_set_value(dev->led_gpio, 0);	/* 打开LED灯 */
	} else if(ledstat == LEDOFF) {
		gpio_set_value(dev->led_gpio, 1);	/* 关闭LED灯 */
	}
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	struct gpioled_dev *dev = filp->private_data;

	/* 释放互斥锁 */
	mutex_unlock(&dev->lock);

	return 0;
}

static struct file_operations gpioled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
	.release = 	led_release,
};

static int __init led_init(void)
{
	int ret = 0;

	/* 初始化互斥体 */
	mutex_init(&gpioled.lock);
	
	/* 设置LED所使用的GPIO */
	/* 1、获取设备节点：gpioled */
	gpioled.nd = of_find_node_by_path("/gpioled");
	if(gpioled.nd == NULL) {
		printk("gpioled node not find!\r\n");
		return -EINVAL;
	} else {
		printk("gpioled node find!\r\n");
	}

	/* 2、 获取设备树中的gpio属性，得到LED所使用的LED编号 */
	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
	if(gpioled.led_gpio < 0) {
		printk("can't get led-gpio");
		return -EINVAL;
	}
	printk("led-gpio num = %d\r\n", gpioled.led_gpio);

	/* 3、设置GPIO1_IO03为输出，并且输出高电平，默认关闭LED灯 */
	ret = gpio_direction_output(gpioled.led_gpio, 1);
	if(ret < 0) {
		printk("can't set gpio!\r\n");
	}

	/* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (gpioled.major) {		
		gpioled.devid = MKDEV(gpioled.major, 0);
		register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	} else {						
		alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		gpioled.major = MAJOR(gpioled.devid);	
		gpioled.minor = MINOR(gpioled.devid);	
	}
	printk("gpioled major=%d,minor=%d\r\n",gpioled.major, gpioled.minor);	
	
	/* 2、初始化cdev */
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

	/* 4、创建类 */
	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class)) {
		return PTR_ERR(gpioled.class);
	}

	/* 5、创建设备 */
	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device)) {
		return PTR_ERR(gpioled.device);
	}
	
	return 0;
}


static void __exit led_exit(void)
{
	
	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT); 

	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");
