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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define BEEP_CNT    1
#define BEEP_NAME   "beep"
#define BEEPOFF     0
#define BEEPON      1

struct beep_dev{

    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int beep_gpio;

};

struct beep_dev beep;

static int beep_open(struct inode *inode, struct file *filp) {

    filp->private_data = &beep;
    return 0;

}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt) {

    int retvalue;
    unsigned char databuf[1];
    unsigned char beepstat;
    struct beep_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0) {

        printk("kernel write failed\r\n");

    }

    beepstat = databuf[0];

    if (beepstat == BEEPON) gpio_set_value(dev->beep_gpio, 0);
    else if (beepstat == BEEPOFF) gpio_set_value(dev->beep_gpio, 1);

    return 0;
}

static int beep_release(struct inode *inode, struct file *filp) {

    return 0;

}

static struct file_operations beep_fops = {

    .owner = THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
    .release = beep_release,

};

static int __init beep_init(void) {

    int ret = 0;

    // 一、设置beep所用的gpio
    // 1. 获取设备节点
    beep.nd = of_find_node_by_path("/beep");
    if (beep.nd == NULL) {

        printk("beep node find failed\r\n");

    } else {

        printk("beep node find success\r\n");

    }

    // 2. 获取设备树中gpio属性，得到beep所使用的gpio编号
    beep.beep_gpio = of_get_named_gpio(beep.nd, "beep-gpio", 0);
    if (beep.beep_gpio < 0) {

        printk("cant get beep-gpio\r\n");
        return -EINVAL;

    } else {

        printk("beep-gpio num = %d\r\n", beep.beep_gpio);

    }

    // 3. 设置gpio5_1为输出，并且输出高电平，默认关闭beep
    ret = gpio_direction_output(beep.beep_gpio, 1);
    if (ret < 0) {

        printk("cant set gpio\r\n");

    }

    // 二、注册设备驱动
    // 1. 创建设备号
    if (beep.major) {

        beep.devid = MKDEV(beep.major, 0);
        register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
        
    } else {

        alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
        beep.major = MAJOR(beep.devid);
        beep.minor = MINOR(beep.devid);
        
    }

    printk("beep major = %d, minor = %d \r\n",beep.major, beep.minor);

    // 2. 初始化cdev
    beep.cdev.owner = THIS_MODULE;
    cdev_init(&beep.cdev, &beep_fops);

    // 3. 添加一个cdev
    cdev_add(&beep.cdev, beep.devid, BEEP_CNT);

    // 4. 创建类
    beep.class  = class_create(THIS_MODULE, BEEP_NAME);
    if (IS_ERR(beep.class)) {

        return -PTR_ERR(beep.class);

    }

    // 5. 创建设备
    beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
        if (IS_ERR(beep.device)) {

        return -PTR_ERR(beep.device);

    }

    return 0;
}

static void __exit beep_exit(void) {

    cdev_del(&beep.cdev);
    unregister_chrdev_region(beep.devid, BEEP_CNT);
    device_destroy(beep.class, beep.devid);
    class_destroy(beep.class);

}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");