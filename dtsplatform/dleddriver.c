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
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LEDDEV_CNT		1			
#define LEDDEV_NAME		"dtsplatled"	
#define LEDOFF 			0
#define LEDON 			1

struct leddev_dev{

    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
	struct device_node *node;
	int led0;				// led0的gpio号

};

struct leddev_dev leddev;

// LED 打开/关闭 函数
void led0_switch(u8 sta) {

    if (sta == LEDON) {
		
		gpio_set_value(leddev.led0, 0);

    } else if (sta == LEDOFF) {

		gpio_set_value(leddev.led0, 1);

    }

}

static int led_open(struct inode *inode, struct file *filp) {

	filp->private_data = &leddev; 
	return 0;

}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt) {

	int retvalue;
	unsigned char databuf[2];
	unsigned char ledstat;

	retvalue = copy_from_user(databuf, buf, cnt);
	if (retvalue < 0) {

		printk("kernel write failed\r\n");
		return -EFAULT;

	}

    ledstat = databuf[0];
    led0_switch(ledstat);

    return 0;   

}

static struct file_operations led_fops = {

	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,

};

// flatform 驱动的probe函数 驱动和设备匹配后自动执行
static int led_probe(struct platform_device *dev) {

    printk("led driver and device has matchedr\r\n");

    // 注册字符设备驱动
    
    if (leddev.major) {
            		
		leddev.devid = MKDEV(leddev.major, 0);
		register_chrdev_region(leddev.devid, LEDDEV_CNT, LEDDEV_NAME);

	} else {

		alloc_chrdev_region(&leddev.devid, 0, LEDDEV_CNT, LEDDEV_NAME);	
		leddev.major = MAJOR(leddev.devid);	

	}

	leddev.cdev.owner = THIS_MODULE;
	cdev_init(&leddev.cdev, &led_fops);  

    cdev_add(&leddev.cdev, leddev.devid, LEDDEV_CNT); 

    leddev.class = class_create(THIS_MODULE, LEDDEV_NAME);
    if (IS_ERR(leddev.class)) {

		return PTR_ERR(leddev.class);

	}

    leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, LEDDEV_NAME);
	if (IS_ERR(leddev.device)) {

		return PTR_ERR(leddev.device);

	}

	// 初始化IO
	leddev.node = of_find_node_by_path("/gpioled");
	if (leddev.node == NULL) {

		printk("gpioled node nost find\r\n");
		return -EINVAL;

	}

	leddev.led0 = of_get_named_gpio(leddev.node, "led-gpio", 0);
	if (leddev.led0 < 0) {

		printk("can't get led-gpio\r\n");
		return -EINVAL;

	}

	gpio_request(leddev.led0, "led0");
	// 设置输出 默认高电平
	gpio_direction_output(leddev.led0, 1);

    return 0;

}

static int led_remove(struct platform_device *dev) {

	gpio_set_value(leddev.led0, 1);
	gpio_free(leddev.led0);

    cdev_del(&leddev.cdev);
	unregister_chrdev_region(leddev.devid, LEDDEV_CNT); 
    
	device_destroy(leddev.class, leddev.devid);
	class_destroy(leddev.class);

	return 0;

}

// 匹配列表
static const struct of_device_id led_of_match[] = {

	{ .compatible = "atkalpha-gpioled" },

	{ 
		// sentinel  
	}

};

// platform驱动结构体 
static struct platform_driver led_driver = {

	.driver		= {
		.name	= "imx6ul-led",			// 驱动名字，用于和设备匹配 
		.of_match_table	= led_of_match, // 设备树匹配表 		 
	},
	.probe		= led_probe,
	.remove		= led_remove,

};

static int __init leddriver_init(void) {

	return platform_driver_register(&led_driver);

}

static void __exit leddriver_exit(void) {

	platform_driver_unregister(&led_driver);

}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");
