// gpmc_pulse.c (TI Linux 5.10 compatible)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/ioport.h>
#include <linux/capability.h>
#include "exp_gpmc_pulse.h"

#define DRV_NAME "gpmc-pulse"

/* Conservative bounds: tune if needed */
#define MIN_WIDTH_NS  200u
#define MAX_WIDTH_NS  10000  /* 10us */
#define MAX_OFFSET    (1024u * 1024u) /* 1MB sanity; actual checked vs allow-size */

/*
 * Driver state
 */
struct gpmc_pulse_dev {
    void __iomem *gpmc;      /* ioremapped allow window */
    phys_addr_t allow_base;
    resource_size_t allow_size;
    spinlock_t lock;
    struct miscdevice misc;
};

static inline void flush_posted_write_8(void __iomem *addr)
{
    /* Read-back is a common way to force posted writes out on many fabrics */
    (void)ioread8(addr);
}

static inline void do_delay_ns(u32 width_ns)
{
    if (width_ns >= 1000) {
        u32 us = width_ns / 1000;
        u32 rem = width_ns % 1000;
        udelay(us);
        if (rem)
            ndelay(rem);
    } else {
        ndelay(width_ns);
    }
}

static bool addr_in_allow_range(const struct gpmc_pulse_dev *d, u64 phys_base, u32 offset, u32 access_width)
{
    u64 addr = phys_base + (u64)offset;
    u64 end  = addr + (u64)access_width - 1;

    u64 lo = (u64)d->allow_base;
    u64 hi = (u64)d->allow_base + (u64)d->allow_size - 1;

    return (addr >= lo) && (end <= hi);
}

static int gpmc_pulse_open(struct inode *inode, struct file *file)
{
    if (!capable(CAP_SYS_RAWIO)) {
        return -EPERM;
    }
    return 0;
}

static long gpmc_pulse_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct gpmc_pulse_dev *d = container_of(f->private_data, struct gpmc_pulse_dev, misc);
    struct exp_pulse_req req;
    unsigned long flags;
    u64 phys_base;
    void __iomem *addr;

    if (!capable(CAP_SYS_RAWIO))
    return -EPERM;

    if (cmd != EXP_PULSE_IOC_DO)
        return -ENOTTY;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
        return -EFAULT;

    /* Validate request */
    if (req.flags != 0)
        return -EINVAL;

    if (req.access_width != 1 && req.access_width != 2 && req.access_width != 4)
        return -EINVAL;

    if (req.width_ns < MIN_WIDTH_NS || req.width_ns > MAX_WIDTH_NS)
        return -EINVAL;

    if (req.offset > MAX_OFFSET)
        return -EINVAL;

    if ((req.access_width == 2 || req.access_width == 4) &&
        (req.offset & (req.access_width - 1)))
        return -EINVAL;

    /* Determine base address */
    phys_base = (req.phys_base != 0) ? req.phys_base : (u64)d->allow_base;

    /* Enforce allow-range */
    if (!addr_in_allow_range(d, phys_base, req.offset, req.access_width))
        return -EPERM;

    /*
     * Mapped the whole allow window at d->gpmc using allow_base.
     * So we only support phys_base == allow_base for direct indexing into d->gpmc.
     */
    if (phys_base != (u64)d->allow_base)
        return -EPERM;

    if (req.offset + req.access_width > d->allow_size)
        return -EINVAL;

    addr = d->gpmc + req.offset;

    /*
     * Critical section: serialize pulses and reduce local IRQ jitter.
     * Keep it tiny. Do NOT sleep here.
     */
    spin_lock_irqsave(&d->lock, flags);

    switch (req.access_width) {
    case 1:
        iowrite8((u8)req.hi_val, addr);
        flush_posted_write_8(addr);
        do_delay_ns(req.width_ns);
        iowrite8((u8)req.lo_val, addr);
        flush_posted_write_8(addr);
        break;

    case 2:
        iowrite16((u16)req.hi_val, addr);
        (void)ioread16(addr);
        do_delay_ns(req.width_ns);
        iowrite16((u16)req.lo_val, addr);
        (void)ioread16(addr);
        break;

    case 4:
        iowrite32((u32)req.hi_val, addr);
        (void)ioread32(addr);
        do_delay_ns(req.width_ns);
        iowrite32((u32)req.lo_val, addr);
        (void)ioread32(addr);
        break;
    }

    spin_unlock_irqrestore(&d->lock, flags);

    return 0;
}

static const struct file_operations gpmc_pulse_fops = {
    .owner          = THIS_MODULE,
    .open           = gpmc_pulse_open,
    .unlocked_ioctl = gpmc_pulse_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = gpmc_pulse_ioctl,
#endif
};

static int gpmc_pulse_probe(struct platform_device *pdev)
{
    struct gpmc_pulse_dev *d;
    struct resource res;
    int ret;

    if (!pdev->dev.of_node)
        return -ENODEV;

    ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
    if (ret) {
        dev_err(&pdev->dev, "of_address_to_resource failed: %d\n", ret);
        return ret;
    }

    d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
    if (!d)
        return -ENOMEM;

    d->allow_base = res.start;              /* phys_addr_t/resource_size_t safe */
    d->allow_size = resource_size(&res);

    if (d->allow_size < 0x1000) {
        dev_err(&pdev->dev, "allow window too small: %pa\n", &d->allow_size);
        return -EINVAL;
    }

    /* Map without claiming exclusivity (coexists with generic-uio) */
    d->gpmc = devm_ioremap(&pdev->dev, d->allow_base, d->allow_size);
    if (!d->gpmc) {
        dev_err(&pdev->dev, "devm_ioremap failed for base=%pa size=%pa\n",
                &d->allow_base, &d->allow_size);
        return -ENOMEM;
    }

    spin_lock_init(&d->lock);

    d->misc.minor  = MISC_DYNAMIC_MINOR;
    d->misc.name   = "gpmc_pulse";
    d->misc.fops   = &gpmc_pulse_fops;
    d->misc.parent = &pdev->dev;
    d->misc.mode   = 0660;

    ret = misc_register(&d->misc);
    if (ret) {
        dev_err(&pdev->dev, "misc_register failed: %d\n", ret);
        return ret;
    }

    platform_set_drvdata(pdev, d);

    dev_info(&pdev->dev, "gpmc-pulse: allow phys=%pa size=%pa gpmc=%p\n",
             &d->allow_base, &d->allow_size, d->gpmc);

    return 0;
}

static int gpmc_pulse_remove(struct platform_device *pdev)
{
    struct gpmc_pulse_dev *d = platform_get_drvdata(pdev);

    if (d)
        misc_deregister(&d->misc);

    return 0;
}

static const struct of_device_id gpmc_pulse_of_match[] = {
    { .compatible = "exp,gpmc-pulse" },
    { }
};
MODULE_DEVICE_TABLE(of, gpmc_pulse_of_match);

static struct platform_driver gpmc_pulse_driver = {
    .probe  = gpmc_pulse_probe,
    .remove = gpmc_pulse_remove,
    .driver = {
        .name = DRV_NAME,
        .of_match_table = gpmc_pulse_of_match,
    },
};

module_platform_driver(gpmc_pulse_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Generic GPMC pulse helper (kernel-timed pulse via ioctl)");
MODULE_AUTHOR("NEXUS");
