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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define NEWLED_CNT      1
#define NEWLED_NAME     "newled"

#define LEDOFF  0 
#define LEDON   1 
 
#define CCM_CCGR1_BASE          (0X020C406C) 
#define SW_MUX_GPIO1_IO03_BASE  (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE  (0X020E02F4)
#define GPIO1_DR_BASE           (0X0209C000)
#define GPIO1_GDIR_BASE         (0X0209C004)
 
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

struct newled_dev{

    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;

};

struct newled_dev newled;

void led_switch(u8 sta) {

    u32 val = 0;
    if (sta == LEDON) {
        
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);

    } else if (sta == LEDOFF) {
        
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);

    }

}

static int led_open(struct inode* inode, struct file *filp) {

    filp->private_data = &newled;
    return 0;

}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt) {

    return 0;

}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt) {

    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0) {
        printk("kernel write failed\r\n");
    }
    ledstat = databuf[0];

    led_switch(ledstat);

    return 0;

}

static int led_release(struct inode *inode, struct file *filp) {

    return 0;

}

static struct file_operations newled_fops = {
    
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,

};

static int __init newled_init(void) {
    
    u32 val = 0;

     // 1. 获取寄存器地址映射
    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
    GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

    // 2. 使能GPIO1时钟
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val, IMX6U_CCM_CCGR1);

    // 3. 设置GPIO1_3的功能
    writel(5, SW_MUX_GPIO1_IO03);

    // 4. 寄存器 SW_PAD_GPIO1_IO03 设置IO属性
    writel(0x10B0, SW_PAD_GPIO1_IO03);
    
    // 5. 默认关闭LED
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3);
    val |= (1 <<3);

    writel(val, GPIO1_GDIR);
    
    // 1. 创建设备号
    if (newled.major) {

        newled.devid = MKDEV(newled.major, 0);
        register_chrdev_region(newled.devid, NEWLED_CNT, NEWLED_NAME);
        
    } else {

        alloc_chrdev_region(&newled.devid, 0, NEWLED_CNT, NEWLED_NAME);
        newled.major = MAJOR(newled.devid);
        newled.minor = MINOR(newled.devid);

    }

    printk("newled major=%d, minor=%d\r\n", newled.major, newled.minor);
    
    // 2. 初始化cdev
    newled.cdev.owner = THIS_MODULE;
    cdev_init(&newled.cdev, &newled_fops);

    // 3. 添加一个cdev
    cdev_add(&newled.cdev, newled.devid, NEWLED_CNT);

    // 4. 创建一个类
    newled.class = class_create(THIS_MODULE, NEWLED_NAME);
    if (IS_ERR(newled.class)) {

        return PTR_ERR(newled.class);
    
    }

    // 5. 创建设备节点
    newled.device = device_create(newled.class, NULL, newled.devid, NULL, NEWLED_NAME);
    if (IS_ERR(newled.device)) {

        return PTR_ERR(newled.device);
    
    }

    return 0;

}

static void __exit newled_exit(void) {

    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    cdev_del(&newled.cdev);
    unregister_chrdev_region(newled.devid, NEWLED_CNT);
    
    device_destroy(newled.class, newled.devid);
    class_destroy(newled.class);

}

module_init(newled_init);
module_exit(newled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");