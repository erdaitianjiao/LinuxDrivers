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
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define IMX6UIRQ_CNT    1
#define IMX6UIRQ_NAME   "noblockio"
#define KEY0VALUE       0X01
#define INVAKEY         0XFF
#define KEY_NUM         1

// 中断IO描述结构体
struct irq_keydesc {

    int gpio;                               // gpio
    int irqnum;                             // 中断号 
    unsigned char value;                    // 按键对应键值
    char name[10];                          // 名字
    irqreturn_t (*handler)(int, void *);    // 中断服务函数    

};

// imx6uirq设备结构体
struct imx6uirq_dev {

    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    atomic_t keyvalue;                          // 有效按键键值
    atomic_t releasekey;                        // 标记是否完成一次完整的按键
    struct timer_list timer;                    // 定义一个定时器
    struct irq_keydesc irqkeydesc[KEY_NUM];     // 按键描述数组
    unsigned char curkeynum;                    // 当前的按键号
    wait_queue_head_t r_wait;                   // 读等待队列头

};

struct imx6uirq_dev imx6uirq;   // 实例化

// 中断服务函数
static irqreturn_t key0_handler(int irq, void *dev_id) {

    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));

    return IRQ_RETVAL(IRQ_HANDLED);

}

// 定时服务函数 用于消抖
void timer_function(unsigned long arg) {

    unsigned char value;
    unsigned char num;
    struct irq_keydesc *keydesc;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->irqkeydesc[num];

    value = gpio_get_value(keydesc->gpio);
    if (value == 0) {

        atomic_set(&dev->keyvalue, keydesc->value);

    } else {

        atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
        atomic_set(&dev->releasekey, 1);

    }
    // 唤醒进程
    if (atomic_read(&dev->releasekey)) {

        wake_up_interruptible(&dev->r_wait);

    }

}

static int keyio_init(void) {

    unsigned char i = 0;
    char name[10];
    int ret = 0;

    imx6uirq.nd = of_find_node_by_path("/key");
    if (imx6uirq.nd == NULL) {

        printk("key node not find\r\n");
        return -EINVAL;

    }
    // 提取gpio
    for (i = 0; i < KEY_NUM; i ++) {

        imx6uirq.irqkeydesc[i].gpio = of_get_named_gpio(imx6uirq.nd, "key-gpio", i);
        if (imx6uirq.irqkeydesc[i].gpio < 0) {

            printk("cant get key\r\n");

        }

    }
    // 初始化key使用的io 并且设置成中断模式
    for (i = 0; i < KEY_NUM; i ++) {

        memset(imx6uirq.irqkeydesc[i].name, 0, sizeof(name));
        sprintf(imx6uirq.irqkeydesc[i].name, "KEY%d",i);
        gpio_request(imx6uirq.irqkeydesc[i].gpio, name);
        gpio_direction_input(imx6uirq.irqkeydesc[i].gpio);
        imx6uirq.irqkeydesc[i].irqnum = irq_of_parse_and_map(imx6uirq.nd, i);
#if 0
        imx6uirq.irqkeydesc[i].irqnum = gpio_to_irq(imx6uirq.irqkeydesc[i].gpio);
#endif
        printk("key%d:gpio=%d, irqnum=%d\r\n", i, 
                                                imx6uirq.irqkeydesc[i].gpio, 
                                                imx6uirq.irqkeydesc[i].irqnum);
    }
    
    // 申请中断
    imx6uirq.irqkeydesc[0].handler = key0_handler;
    imx6uirq.irqkeydesc[0].value = KEY0VALUE;
    
    for (i = 0; i < KEY_NUM; i ++) {

        ret = request_irq(imx6uirq.irqkeydesc[i].irqnum,
                          imx6uirq.irqkeydesc[i].handler,
                          IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                          imx6uirq.irqkeydesc[i].name,
                          &imx6uirq
                        );
        if (ret < 0) {

            printk("irq %d request filed\r\n", imx6uirq.irqkeydesc[i].irqnum);
            return -EFAULT;

        }

    }
    
    // 创建定时器
    init_timer(&imx6uirq.timer);
    imx6uirq.timer.function = timer_function;
    
    init_waitqueue_head(&imx6uirq.r_wait);
    return 0; 
    
}

static int imx6uirq_open(struct inode *inode, struct file *filp) {

    filp->private_data = &imx6uirq;
    return 0;

}

static ssize_t imx6uirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt) {

    int ret = 0;
    unsigned char keyvalue = 0;
    unsigned char releasekey = 0;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;
    
    if (filp->f_flags & O_NONBLOCK) {

        if (atomic_read(&dev->releasekey) == 0) {

            return -EAGAIN;

        }
    } else {
    
        ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
        if (ret) {

            goto wait_error;

        }

    }

    keyvalue = atomic_read(&dev->keyvalue);
    releasekey = atomic_read(&dev->releasekey);

    if (releasekey) {

        if (keyvalue & 0x80) {

            keyvalue &= ~0x80;
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));

        } else {

            goto data_error;

        }
        atomic_set(&dev->releasekey, 0);

    } else {

        goto data_error;

    }
    return 0;

wait_error:
    return ret;

data_error:
    return -EINVAL;
}


// poll函数 用于处理非阻塞访问
unsigned int imx6uirq_poll(struct file *filp, struct poll_table_struct *wait) { 

    unsigned int mask = 0;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);

    if (atomic_read(&dev->releasekey)) {

        mask = POLLIN | POLLRDNORM; 

    }
    return mask;

}

    


static struct file_operations imx6uirq_fops = {

    .owner = THIS_MODULE,
    .open = imx6uirq_open,
    .read = imx6uirq_read,
    .poll = imx6uirq_poll,

};


static int __init imx6uirq_init(void) {

    if (imx6uirq.major) {

        imx6uirq.devid = MKDEV(imx6uirq.major, 0);
        register_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);

    } else {

        alloc_chrdev_region(&imx6uirq.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
        imx6uirq.major = MAJOR(imx6uirq.devid);
        imx6uirq.minor = MINOR(imx6uirq.devid);

    }

    cdev_init(&imx6uirq.cdev, &imx6uirq_fops);
    cdev_add(&imx6uirq.cdev, imx6uirq.devid, IMX6UIRQ_CNT);

    imx6uirq.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirq.class)) {

        return PTR_ERR(imx6uirq.class);

    }

    imx6uirq.device = device_create(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirq.device)) {

        return PTR_ERR(imx6uirq.device);

    }

    atomic_set(&imx6uirq.keyvalue, INVAKEY);
    atomic_set(&imx6uirq.releasekey, 0);
    keyio_init();

    return 0;   

}
static void __exit imx6uirq_exit(void) {
    unsigned int i = 0;
    
    del_timer_sync(&imx6uirq.timer);

    for (i = 0; i < KEY_NUM; i ++) {

        free_irq(imx6uirq.irqkeydesc[i].irqnum, &imx6uirq);
        gpio_free(imx6uirq.irqkeydesc[i].gpio);

    }
    
    cdev_del(&imx6uirq.cdev);
    unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);
    device_destroy(imx6uirq.class, imx6uirq.devid);
    class_destroy(imx6uirq.class);

}

module_init(imx6uirq_init);
module_exit(imx6uirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");
