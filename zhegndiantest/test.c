#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init abc_init(void) {

    printk("chrdevbase_init\r\n");
    return 0;

}


static void __exit abc_exit(void) {

    printk("chrdevbase_exxt\r\n");

}

module_init(abc_init);
module_exit(abc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tianjiao");
