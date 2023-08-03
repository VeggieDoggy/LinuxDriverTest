#include "asm-generic/gpio.h"
#include "asm/gpio.h"
#include "linux/err.h"
#include "linux/gpio.h"
#include "linux/gpio/driver.h"
#include "linux/irqreturn.h"
#include "linux/kdev_t.h"
#include "linux/printk.h"
#include "linux/uaccess.h"
#include "linux/wait.h"
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


static int major = 0;
static struct class *sr501_class;
static struct gpio_desc *sr501_gpio;
static struct device *sr501_dev;
static int irq;
static int sr501_data = 0;
static wait_queue_head_t sr501_wq;

static ssize_t sr501_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset) {
    unsigned long ret;
    int len = (size < 4)? size : 4;

    /* 1. If there is data, copy_to_user */
    /* 2. If there is no data, sleep: attach to a linked list */
    wait_event_interruptible(sr501_wq, sr501_data);
    ret = copy_to_user(buf, &sr501_data, len);
    sr501_data = 0;
    return len;

} 

static unsigned int sr501_drv_poll(struct file *fp, poll_table * wait) {
    return 0;
}

static struct file_operations sr501_fops = {
    .owner = THIS_MODULE,
    .read = sr501_drv_read,
    .poll = sr501_drv_poll,
};

static irqreturn_t sr501_isr(int irq, void *dev_id)
{
    int val;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* 1. Record data */
    sr501_data = gpiod_get_value(sr501_gpio);
    /* 2. Awake APP: Use same linked list to awake APP */
    wake_up(&sr501_wq);


	return IRQ_WAKE_THREAD;
}


static int sr501_probe(struct platform_device *pdev) {
    int ret;

    /* 1. Get device information */
    sr501_gpio = gpiod_get(&pdev->dev, NULL, 0);

    if(IS_ERR(sr501_gpio)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        dev_err(&pdev->dev, "Failed to get GPIO for sr501");
        return PTR_ERR(sr501_gpio);
    }

    /* 2. Set direction */
    gpiod_direction_input(sr501_gpio);

    irq = gpiod_to_irq(sr501_gpio);
    ret = request_irq(irq, sr501_isr, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "sr501", NULL);

    /* 3. device_create */
    sr501_dev = device_create(sr501_class, NULL, MKDEV(major, 0), NULL, "/dev/sr501");
    if(IS_ERR(sr501_dev)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        dev_err(&pdev->dev, "Failed to create decive for sr501");
        device_destroy(sr501_class, MKDEV(major, 0));f
        return PTR_ERR(sr501_gpio);
    }


}

static int sr501_remove(struct platform_device *pdev) {
    remove_wait_queue(&sr501_wq, struct wait_queue_entry *wq_entry);

    device_destroy(sr501_class, MKDEV(major, 0));
    class_destroy(sr501_class);
    free_irq(irq);
    gpiod_put(sr501_gpio);
}

static struct of_device_id ask100_sr501[] = {
    { .compatible = "100ask,sr501"},
    // Empty array item means the end of array
    { },
};

static struct platform_driver sr501_driver = {
    .probe = sr501_probe,
    .remove = sr501_remove,
    .driver = {
        .name = "100ask_sr501",
        .of_match_table = ask100_sr501,
    }
};

/* 在入口函数 */
static int __init sr501_drv_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    major = register_chrdev(0, "sr501", &sr501_fops);  /* /dev/gpio_desc */
    
	sr501_class = class_create(THIS_MODULE, "sr501_class");
    if(IS_ERR(sr501_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "sr501");
        return PTR_ERR(sr501_class);
    }

    init_waitqueue_head(&sr501_wq);

    err = platform_driver_register(&sr501_driver);

    return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数                 */
static void __exit sr501_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    
    platform_driver_unregister(&sr501_driver);
}




module_init(sr501_drv_init);
module_exit(sr501_drv_exit);

MODULE_LICENSE("GPL");