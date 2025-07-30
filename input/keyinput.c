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
#include <linux/input.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEYINPUT_CNT		1			
#define KEYINPUT_NAME		"keyinput"	

#define KEY0VALUE			0X01		
#define INVAKEY				0XFF		
#define KEY_NUM				1			

// 中断IO描述结构体
struct irq_keydesc {

   	int gpio;								
	int irqnum;								// 中断号
	unsigned char value;					
	char name[10];							
	irqreturn_t (*handler)(int, void *);	// 中断服务函数

};

struct keyinput_dev {

    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    struct timer_list timer;                    // 定时器结构体
    struct irq_keydesc irqkeydesc[KEY_NUM];
    unsigned char curkeynum;
    atomic_t keyvalue;
    struct input_dev *inputdev;                 // input 结构体

};

struct keyinput_dev keyinputdev;

// 中断服务函数 用于开启定时器
static irqreturn_t key0_handler(int irq, void *dev_id) {

    struct keyinput_dev *dev = (struct keyinput_dev *)dev_id;

	dev->curkeynum = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));	 //10ms定时

	return IRQ_RETVAL(IRQ_HANDLED);

}

// 定时器服务函数 循环执行
void timer_function(unsigned long arg) {

    unsigned char value;
    unsigned char num;
    struct irq_keydesc *keydesc;
    struct keyinput_dev *dev = (struct keyinput_dev *)arg;

    // 读取按键值
    num = dev->curkeynum;
    keydesc = &dev->irqkeydesc[num];
    value = gpio_get_value(keydesc->gpio);

    // 按下按键
    if (value == 0) {

        // 上报按键值
        input_report_key(dev->inputdev, keydesc->value, 1);
        input_sync(dev->inputdev);

    } else {

        input_report_key(dev->inputdev, keydesc->value, 0);
        input_sync(dev->inputdev);

    }

}



static int keyio_init(void) {

    unsigned char i = 0;
	char name[10];
	int ret = 0;

	keyinputdev.nd = of_find_node_by_path("/key");
    if (keyinputdev.nd == NULL) {

        printk("key node not find\r\n");
        return -EINVAL;

    }

    // 提取GPIO
	for (i = 0; i < KEY_NUM; i++) {

		keyinputdev.irqkeydesc[i].gpio = of_get_named_gpio(keyinputdev.nd ,"key-gpio", i);
		if (keyinputdev.irqkeydesc[i].gpio < 0) {

			printk("can't get key%d\r\n", i);

		}

	}

    // 初始化key使用gpio 设置成中断模式
    for (i = 0; i < KEY_NUM; i++) {

		memset(keyinputdev.irqkeydesc[i].name, 0, sizeof(name));	// 缓冲区清零 
		sprintf(keyinputdev.irqkeydesc[i].name, "KEY%d", i);		
		gpio_request(keyinputdev.irqkeydesc[i].gpio, name);
		gpio_direction_input(keyinputdev.irqkeydesc[i].gpio);

		keyinputdev.irqkeydesc[i].irqnum = irq_of_parse_and_map(keyinputdev.nd, i);

	}

    // 中断申请
	keyinputdev.irqkeydesc[0].handler = key0_handler;
	keyinputdev.irqkeydesc[0].value = KEY_0;

    for (i = 0; i < KEY_NUM; i++) {

		ret = request_irq(keyinputdev.irqkeydesc[i].irqnum, keyinputdev.irqkeydesc[i].handler, 
		                 IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, keyinputdev.irqkeydesc[i].name, &keyinputdev);
		if(ret < 0){

			printk("irq %d request failed!\r\n", keyinputdev.irqkeydesc[i].irqnum);
			return -EFAULT;

		}

	}

    // 创建定时器
	init_timer(&keyinputdev.timer);
	keyinputdev.timer.function = timer_function;    

    // 申请input_dev
    keyinputdev.inputdev = input_allocate_device();
	keyinputdev.inputdev->name = KEYINPUT_NAME;

#if 0

	// 初始化input_dev，设置产生哪些事件 
	__set_bit(EV_KEY, keyinputdev.inputdev->evbit);	// 设置产生按键事件          
	__set_bit(EV_REP, keyinputdev.inputdev->evbit);	// 重复事件，比如按下去不放开，就会一直输出信息 		 

	// 初始化input_dev，设置产生哪些按键 
	__set_bit(KEY_0, keyinputdev.inputdev->keybit);	

#endif

#if 0

	keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	keyinputdev.inputdev->keybit[BIT_WORD(KEY_0)] |= BIT_MASK(KEY_0);

#endif

    keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	input_set_capability(keyinputdev.inputdev, EV_KEY, KEY_0);

    // 注册输入设备
    ret = input_register_device(keyinputdev.inputdev);
	if (ret) {

		printk("register input device failed\r\n");
		return ret;

	}

    return 0;
}

static int __init keyinput_init(void) {

	keyio_init();
	return 0;

}


static void __exit keyinput_exit(void) {

	unsigned int i = 0;

    // 删除定时器
	del_timer_sync(&keyinputdev.timer);	
		
	// 释放中断 
	for (i = 0; i < KEY_NUM; i++) {

		free_irq(keyinputdev.irqkeydesc[i].irqnum, &keyinputdev);

	}

	// 释放input_dev 
	input_unregister_device(keyinputdev.inputdev);
	input_free_device(keyinputdev.inputdev);

}

module_init(keyinput_init);
module_exit(keyinput_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");