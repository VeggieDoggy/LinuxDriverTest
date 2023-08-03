#include "asm-generic/errno-base.h"
#include "asm-generic/param.h"
#include <linux/wait.h>
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
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>




static int major;
static struct class *dht11_class;
static struct gpio_desc *dht11_trig_gpiod;
static struct gpio_desc *dht11_echo_gpiod;
static unsigned int irq;
static int dht11_data_ns = 0;
static wait_queue_head_t dht11_wq;


static ssize_t dht11_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset) {
    int timeout;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* send 10us high level */
    gpiod_set_value(dht11_trig_gpiod, 1);
    udelay(12);
    gpiod_set_value(dht11_trig_gpiod, 0);

    /* wait for data */
    timeout = wait_event_interruptible_timeout(dht11_wq, dht11_data_ns, HZ);

    if(timeout){
        copy_to_user(buf, &dht11_data_ns, 4);
        dht11_data_ns = 0;
        return 4;
    } else {
        return -EAGAIN;
    }

    
}
static unsigned int dht11_drv_poll(struct file *fp, poll_table * wait) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static struct file_operations dht11_fops = {
    .owner = THIS_MODULE,
    .read = dht11_drv_read,
    .poll = dht11_drv_poll,
};

static irqreturn_t dht11_isr (int irq, void *dev_id) {
    int val = gpiod_get_value(dht11_echo_gpiod);

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    if (val) {  // Rising
        dht11_data_ns = ktime_get_ns(); // get kernel launch time, pause when kernel hang
    } else {    // Falling
        dht11_data_ns = ktime_get_ns() - dht11_data_ns;

        wake_up(&dht11_wq);
    }

    return IRQ_WAKE_THREAD;
}

static int dht11_probe (struct platform_device *pdev) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    dht11_trig_gpiod = gpiod_get(&pdev->dev, "trig", GPIOD_OUT_LOW);
    dht11_echo_gpiod = gpiod_get(&pdev->dev, "echo", GPIOD_IN);

    irq = gpiod_to_irq(dht11_echo_gpiod);
    request_irq(irq, dht11_isr, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "dht11", NULL);

    device_create(dht11_class, NULL, MKDEV(major, 0), NULL, "dht11");

    return 0;
}

static int dht11_remove (struct platform_device *pdev) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(dht11_class, MKDEV(major,0));

    free_irq(irq, dht11_isr);

    gpiod_put(dht11_echo_gpiod);
    gpiod_put(dht11_trig_gpiod);

    return 0;
}

static const struct of_device_id liuq_dht11[] = {
    { .compatible = "liuq,dht11" },
    { },
};

static struct platform_driver dht11s_driver = {
    .probe = dht11_probe,
    .remove = dht11_remove,
    .driver = {
        .name = "my_dht11",
        .of_match_table = liuq_dht11,
    }
};

static int __init dht11_init(void) {
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    major = register_chrdev(0, "dht11", &dht11_fops);
    dht11_class = class_create(THIS_MODULE, "dht11_class");

    err = platform_driver_register(&dht11s_driver);

    return err;
}

static void __exit dht11_exit(void) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    platform_driver_unregister(&dht11s_driver);
    class_destroy(dht11_class);
    unregister_chrdev(major, "dht11");
}

module_init(dht11_init);
module_exit(dht11_exit);

MODULE_LICENSE("GPL");