#include "asm-generic/errno-base.h"
#include "asm-generic/param.h"
#include "asm/delay.h"
#include "asm/irqflags.h"
#include "linux/printk.h"
#include "linux/stddef.h"
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
static struct gpio_desc *dht11_gpiod;
//static unsigned int irq;
//static int dht11_data = 0;
//static wait_queue_head_t dht11_wq;


static void dht11_reset (void) {    
    gpiod_direction_output(dht11_gpiod, 1);
    //printk("GPIO direction = %i; %s %s line %d\n",gpiod_get_direction(dht11_gpiod), __FILE__, __FUNCTION__, __LINE__);
}

static void dht11_start (void) {  
    mdelay(30);
    gpiod_set_value(dht11_gpiod, 0);
    mdelay(20);
    gpiod_set_value(dht11_gpiod, 1);
    udelay(40);
    gpiod_direction_input(dht11_gpiod);
    //printk("GPIO direction = %i; %s %s line %d\n",gpiod_get_direction(dht11_gpiod), __FILE__, __FUNCTION__, __LINE__);
    udelay(2);
}

static int dht11_wait_for_ready (void) {
    int timeout_us = 200;
    int us = 0;

    /* wait for low level */
    while (gpiod_get_value(dht11_gpiod) && --timeout_us) {
        udelay(1);
    }
    if (!timeout_us) {
        printk("ERROR: Time out! GPIO value = %i; %s %s line %d\n",gpiod_get_value(dht11_gpiod), __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
    /* low level now*/
    /* wait for high level */
    timeout_us = 120;
    while (!gpiod_get_value(dht11_gpiod) && --timeout_us) {
        udelay(1);
        us++;
    }
    if (!timeout_us) {
        printk("ERROR: time out! %s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }

    /* high level now */

    return 0;
}

static int dht11_read_bytes (unsigned char *buf) {
    int i;
    unsigned char data = 0;
    int timeout_us;
    int count_us = 0;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    for (i = 0; i<8; i++) {
        /* low level now*/
        /* wait for high level */
        timeout_us = 400;
        while (!gpiod_get_value(dht11_gpiod) && --timeout_us) {
            udelay(1);
        }
        if (!timeout_us) {
            printk("ERROR: time out! %s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
            return -1;
        }
        
        /* wait for low level */
        timeout_us = 200;
        count_us = 0;
        while (gpiod_get_value(dht11_gpiod) && --timeout_us) {
            udelay(1);
            count_us++;
        }
        if (!timeout_us) {
            printk("ERROR: Time out! %s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
            return -1;
        }
        if (count_us > 45) {
			/* get bit 1 */
			data = (data << 1) | 1;
		} else {
			/* get bit 0 */
			data = (data << 1) | 0;
		}        
    }

    return 0;
}

static ssize_t dht11_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset) {
    unsigned long flags;
    int i;
    unsigned char data[5];

    if (size != 4)
        return -EINVAL;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    local_irq_save(flags);

    /* 1. Host send start high level signal */
    dht11_reset();
    dht11_start();
    /* 2. Host pulls up and wait for sensor's response */
    if(dht11_wait_for_ready()) {
        local_irq_restore(flags);
        return -EAGAIN;
    }
    /* 3. 5 Bytes Data transmission */
    for (i=0; i<5; i++) {
        if(dht11_read_bytes(&data[i])) {
            local_irq_restore(flags);
            return -EAGAIN;
        }
    }

    dht11_reset();
    
    local_irq_restore(flags);

    /* 4. Verify data based on checksum */
    if (data[4] != (data[0] + data[1] + data[2] + data[3])) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
    }

    /* 5. Copy to user */
    /* data[0]/data[1] Humidity */
    /* data[2]/data[3] Temperature */

    copy_to_user(buf, data, 4);
    return 4;
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

// static irqreturn_t dht11_isr (int irq, void *dev_id) {
//     //int val = gpiod_get_value(dht11_gpiod);
//     printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
//     wake_up(&dht11_wq);


//     return IRQ_HANDLED;
// }

static int dht11_probe (struct platform_device *pdev) {
    struct device *dht11_dev;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    dht11_gpiod = gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(dht11_gpiod)) {
		printk("Failed to get gpiod! %s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	}

    //irq = gpiod_to_irq(dht11_gpiod);
    //request_irq(irq, dht11_isr, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "dht11", NULL);

    dht11_dev = device_create(dht11_class, NULL, MKDEV(major, 0), NULL, "dht11");
    if(IS_ERR(dht11_dev)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        dev_err(&pdev->dev, "Failed to create decive for dht11");
        device_destroy(dht11_class, MKDEV(major, 0));
        return PTR_ERR(dht11_gpiod);
    }

    return 0;
}

static int dht11_remove (struct platform_device *pdev) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(dht11_class, MKDEV(major,0));

    //free_irq(irq, dht11_isr);

    gpiod_put(dht11_gpiod);

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
    if(IS_ERR(dht11_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "dht11");
        return PTR_ERR(dht11_class);
    }

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