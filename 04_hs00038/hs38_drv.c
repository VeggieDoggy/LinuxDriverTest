#include "asm-generic/errno-base.h"
#include "linux/printk.h"
#include <linux/module.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <asm/current.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/version.h>

static int major;
static struct class *hs38_class;
static struct gpio_desc *hs38_gpiod;
static int irq;
static int is_data = 0;  
static wait_queue_head_t hs38_wq;
static u64 hs38_edge_time[68];
static int hs38_edge_cnt = 0;

int is_nec_code(u64 edge_time, int edge_cnt) {

	u64 all_time = hs38_edge_time[65]-hs38_edge_time[0];
	u64 boot_low_time = hs38_edge_time[1]-hs38_edge_time[0];
	u64 boot_high_time = hs38_edge_time[2]-hs38_edge_time[1];

	if (all_time>=23000000 && all_time <=32000000)
	{
		if(boot_low_time>=8500000 && boot_low_time<=9500000 && boot_high_time>=4000000 && boot_high_time<=5000000) {
			return 1;
		} else {
			printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
			return -EAGAIN;
		}
		
	} else {
		return -EAGAIN;
	}
}

int hs38_parse_data(char *data)
{
	int i, j =0;
	
	int m = 3;

	printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);

	if(is_nec_code(*hs38_edge_time, hs38_edge_cnt)) {
		for (i = 0; i < 4; i++)
		{
			data[i] = 0;
			for (j = 0; j < 8; j++)
			{
				data[i] <<= 1;
				if (hs38_edge_time[m+1] - hs38_edge_time[m] >= 600000)
					data[i] |= 1;
				m += 2;	
			}
		}

		if (~data[0] == data[1] && ~data[2] == data[3]) {
			return 0;
		}
	}
	return -1;
}
	

static irqreturn_t hs38_isr(int irq, void *dev_id)
{
	printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
	/* 1. 记录时间 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0))
	hs38_edge_time[hs38_edge_cnt++] = ktime_get_boottime_ns();
#else
	hs38_edge_time[hs38_edge_cnt++] = ktime_get_boot_ns();
#endif
	if (hs38_edge_cnt >= 66)
	{	
		/* 2. 唤醒APP:去同一个链表把APP唤醒 */
		is_data = 1;
		wake_up(&hs38_wq);
	}
	
	return IRQ_HANDLED; // IRQ_WAKE_THREAD;
}



/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t hs38_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	unsigned char data[4];
	int timeout;
	int err;

	if (size != 4)
		return -EINVAL;

	//gpiod_set_value(hs38_test_pin, 0);

	/* 1. register irq and waiting for boot code*/

	hs38_edge_cnt = 0;
	is_data = 0;

	err = request_irq(irq, hs38_isr, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "liuq_hs38", NULL);
	if (err) {
		pr_warn("Unable to request IRQ.\n");
	}

	timeout = wait_event_timeout(hs38_wq, is_data, HZ);

	free_irq(irq, NULL);
	
	printk("hs38_edge_cnt: %i",hs38_edge_cnt);

	if (!hs38_parse_data(data))
	{
		err = copy_to_user(buf, data, 4);

		return 4;
	}
	else
	{
		return -EAGAIN;
	}

}

static unsigned int hs38_drv_poll(struct file *fp, poll_table * wait)
{
//	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
//	poll_wait(fp, &hs38_wait, wait);
	return 0;
}



/* 定义自己的file_operations结构体                                              */
static struct file_operations hs38_fops = {
	.owner	 = THIS_MODULE,
	.read    = hs38_drv_read,
	.poll    = hs38_drv_poll,
};




/* 1. 从platform_device获得GPIO
 * 2. gpio=>irq
 * 3. request_irq
 */
static int hs38_probe(struct platform_device *pdev)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 1. 获得硬件信息 */
	hs38_gpiod = gpiod_get(&pdev->dev, NULL, GPIOD_OUT_HIGH);
	if (IS_ERR(hs38_gpiod))
	{
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	}

	irq = gpiod_to_irq(hs38_gpiod);

	/* 2. device_create */
	device_create(hs38_class, NULL, MKDEV(major, 0), NULL, "myhs38");

	return 0;
}

static int hs38_remove(struct platform_device *pdev)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	device_destroy(hs38_class, MKDEV(major, 0));
//	free_irq(irq, NULL);
	gpiod_put(hs38_gpiod);
//	gpiod_put(hs38_test_pin);

	return 0;
}


static const struct of_device_id ask100_hs38[] = {
    { .compatible = "liuq,gpiodev" },
    { },
};

/* 1. 定义platform_driver */
static struct platform_driver hs38_driver = {
    .probe      = hs38_probe,
    .remove     = hs38_remove,
    .driver     = {
        .name   = "liuq_hs38",
        .of_match_table = ask100_hs38,
    },
};

/* 2. 在入口函数注册platform_driver */
static int __init hs38_init(void)
{
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	/* 注册file_operations 	*/
	major = register_chrdev(0, "hs38", &hs38_fops);
	if (!major)
		printk(KERN_ERR "Unable to get major %d for gpiodevices\n", major);

	hs38_class = class_create(THIS_MODULE, "hs38_class");
	if (IS_ERR(hs38_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "hs38");
		return PTR_ERR(hs38_class);
	}

	init_waitqueue_head(&hs38_wq);

    err = platform_driver_register(&hs38_driver);
	if (err)
		pr_notice("hs38 initialization failed\n");
	else
		pr_notice("hs38 initialization done\n");

	return err;
}

/* 3. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 *     卸载platform_driver
 */
static void __exit hs38_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    platform_driver_unregister(&hs38_driver);
	class_destroy(hs38_class);
	unregister_chrdev(major, "hs38");
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(hs38_init);
module_exit(hs38_exit);

MODULE_LICENSE("GPL");


