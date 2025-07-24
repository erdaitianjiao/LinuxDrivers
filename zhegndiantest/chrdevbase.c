#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
/***************************************************************
Copyright 漏 ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
鏂囦欢鍚?	: chrdevbase.c
浣滆€?  	: 宸﹀繝鍑?
鐗堟湰	   	: V1.0
鎻忚堪	   	: chrdevbase椹卞姩鏂囦欢銆?
鍏朵粬	   	: 鏃?
璁哄潧 	   	: www.openedv.com
鏃ュ織	   	: 鍒濈増V1.0 2019/1/30 宸﹀繝鍑垱寤?
***************************************************************/

#define CHRDEVBASE_MAJOR	200				/* 涓昏澶囧彿 */
#define CHRDEVBASE_NAME		"chrdevbase" 	/* 璁惧鍚?    */

static char readbuf[100];		/* 璇荤紦鍐插尯 */
static char writebuf[100];		/* 鍐欑紦鍐插尯 */
static char kerneldata[] = {"kernel data!"};

/*
 * @description		: 鎵撳紑璁惧
 * @param - inode 	: 浼犻€掔粰椹卞姩鐨刬node
 * @param - filp 	: 璁惧鏂囦欢锛宖ile缁撴瀯浣撴湁涓彨鍋歱rivate_data鐨勬垚鍛樺彉閲?
 * 					  涓€鑸湪open鐨勬椂鍊欏皢private_data鎸囧悜璁惧缁撴瀯浣撱€?
 * @return 			: 0 鎴愬姛;鍏朵粬 澶辫触
 */
static int chrdevbase_open(struct inode *inode, struct file *filp)
{
	//printk("chrdevbase open!\r\n");
	return 0;
}

/*
 * @description		: 浠庤澶囪鍙栨暟鎹?
 * @param - filp 	: 瑕佹墦寮€鐨勮澶囨枃浠?鏂囦欢鎻忚堪绗?
 * @param - buf 	: 杩斿洖缁欑敤鎴风┖闂寸殑鏁版嵁缂撳啿鍖?
 * @param - cnt 	: 瑕佽鍙栫殑鏁版嵁闀垮害
 * @param - offt 	: 鐩稿浜庢枃浠堕鍦板潃鐨勫亸绉?
 * @return 			: 璇诲彇鐨勫瓧鑺傛暟锛屽鏋滀负璐熷€硷紝琛ㄧず璇诲彇澶辫触
 */
static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;
	
	/* 鍚戠敤鎴风┖闂村彂閫佹暟鎹?*/
	memcpy(readbuf, kerneldata, sizeof(kerneldata));
	retvalue = copy_to_user(buf, readbuf, cnt);
	if(retvalue == 0){
		printk("kernel senddata ok!\r\n");
	}else{
		printk("kernel senddata failed!\r\n");
	}
	
	//printk("chrdevbase read!\r\n");
	return 0;
}

/*
 * @description		: 鍚戣澶囧啓鏁版嵁 
 * @param - filp 	: 璁惧鏂囦欢锛岃〃绀烘墦寮€鐨勬枃浠舵弿杩扮
 * @param - buf 	: 瑕佸啓缁欒澶囧啓鍏ョ殑鏁版嵁
 * @param - cnt 	: 瑕佸啓鍏ョ殑鏁版嵁闀垮害
 * @param - offt 	: 鐩稿浜庢枃浠堕鍦板潃鐨勫亸绉?
 * @return 			: 鍐欏叆鐨勫瓧鑺傛暟锛屽鏋滀负璐熷€硷紝琛ㄧず鍐欏叆澶辫触
 */
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;
	/* 鎺ユ敹鐢ㄦ埛绌洪棿浼犻€掔粰鍐呮牳鐨勬暟鎹苟涓旀墦鍗板嚭鏉?*/
	retvalue = copy_from_user(writebuf, buf, cnt);
	if(retvalue == 0){
		printk("kernel recevdata:%s\r\n", writebuf);
	}else{
		printk("kernel recevdata failed!\r\n");
	}
	
	//printk("chrdevbase write!\r\n");
	return 0;
}

/*
 * @description		: 鍏抽棴/閲婃斁璁惧
 * @param - filp 	: 瑕佸叧闂殑璁惧鏂囦欢(鏂囦欢鎻忚堪绗?
 * @return 			: 0 鎴愬姛;鍏朵粬 澶辫触
 */
static int chrdevbase_release(struct inode *inode, struct file *filp)
{
	//printk("chrdevbase release锛乗r\n");
	return 0;
}

/*
 * 璁惧鎿嶄綔鍑芥暟缁撴瀯浣?
 */
static struct file_operations chrdevbase_fops = {
	.owner = THIS_MODULE,	
	.open = chrdevbase_open,
	.read = chrdevbase_read,
	.write = chrdevbase_write,
	.release = chrdevbase_release,
};

/*
 * @description	: 椹卞姩鍏ュ彛鍑芥暟 
 * @param 		: 鏃?
 * @return 		: 0 鎴愬姛;鍏朵粬 澶辫触
 */
static int __init chrdevbase_init(void)
{
	int retvalue = 0;

	/* 娉ㄥ唽瀛楃璁惧椹卞姩 */
	retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
	if(retvalue < 0){
		printk("chrdevbase driver register failed\r\n");
	}
	printk("chrdevbase init!\r\n");
	return 0;
}

/*
 * @description	: 椹卞姩鍑哄彛鍑芥暟
 * @param 		: 鏃?
 * @return 		: 鏃?
 */
static void __exit chrdevbase_exit(void)
{
	/* 娉ㄩ攢瀛楃璁惧椹卞姩 */
	unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
	printk("chrdevbase exit!\r\n");
}

/* 
 * 灏嗕笂闈袱涓嚱鏁版寚瀹氫负椹卞姩鐨勫叆鍙ｅ拰鍑哄彛鍑芥暟 
 */
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

/* 
 * LICENSE鍜屼綔鑰呬俊鎭?
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");

