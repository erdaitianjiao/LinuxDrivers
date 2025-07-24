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
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_CNT       1
#define TIMER_NAME      "timer"
#define CLOSE_CMD       (_IO(0XEF, 0X1))
#define OPEN_CMD        (_IO(0XEF, 0X2))
#define SETPERIOD_CMD   (_IO(0XEF, 0X2))
#define LEDON           1
#define LEDOFF          0

struct timer_dev {

    dev_t devid;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;
    int timeperiod;
    struct timer_list timer;
    spinlock_t lock;

};

struct timer_dev timerdev;

static int led_init(void) {

    int ret = 0;
    timerdev.nd = of_find_node_by_path("/gpioled");
    if (timedev.nd == NULL) {

        printk("cant find gpioled\r\n");
        return -EINVAL;

    }
    timerdev.led_gpio = of_get_named_gpio(timerdev.nd, "led-gpio", 0);
    if (timerdev.led_gpio < 0) {

        printk("cant get led\r\n");

    }
    
    return 0;
} 

static timer_open(struct inode *inode, struct file *filp) {

    int ret = 0;
    filp->private_data = &timerdev;

    timerdev.timeperiod = 1000;
    ret = led_init();

    if (ret < 0) {

        return ret;

    }

    return 0;

}

static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

    struct timer_dev *dev = (struct timer_dev *)filp->private_data;
    int timerperiod;
    unsigned long flags;
    
    switch (cmd) {

        case CLOSE_CMD:
            del_timer_sync(&dev->timer);
            break;
        case OPEN_CMD:
            spin_lock_irqsace(&devv->lock, flags);
            timerperiod = dev->timeperiod
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerperiod));
            break;
        case SETPERIOD_CMD:
            spin_lock_irqsave(&dev->lock, flasg);
            dev->timeperiod = arg;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
            break;
        default:
            break;

    }

    return 0;

}

static struct file_operations timer_fops = {

    .owmer = THIS_MODULE;
    .open = timer_open;

}