#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

#define TILE_SIZE (6 * 64 * 64 * 2)   /* 49152 bytes */

static unsigned char tile_buf[TILE_SIZE];
static DEFINE_MUTEX(tile_lock);

static ssize_t tile_read(struct file *f, char __user *ubuf,
                         size_t len, loff_t *off)
{
    ssize_t ret;

    mutex_lock(&tile_lock);
    ret = simple_read_from_buffer(ubuf, len, off, tile_buf, TILE_SIZE);
    mutex_unlock(&tile_lock);
    return ret;
}

static ssize_t tile_write(struct file *f, const char __user *ubuf,
                          size_t len, loff_t *off)
{
    ssize_t ret;

    mutex_lock(&tile_lock);
    ret = simple_write_to_buffer(tile_buf, TILE_SIZE, off, ubuf, len);
    mutex_unlock(&tile_lock);

    if (ret > 0 && *off == TILE_SIZE)
        pr_info("tile: new tile loaded\n");
    return ret;
}

static const struct file_operations tile_fops = {
    .owner  = THIS_MODULE,
    .read   = tile_read,
    .write  = tile_write,
    .llseek = default_llseek,
};

static struct miscdevice tile_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "tile",
    .fops  = &tile_fops,
    .mode  = 0666,
};

static int __init tile_init(void)
{
    int ret = misc_register(&tile_dev);

    if (ret)
        pr_err("tile: register failed (%d)\n", ret);
    else
        pr_info("tile: /dev/tile ready (%d bytes)\n", TILE_SIZE);
    return ret;
}

static void __exit tile_exit(void)
{
    misc_deregister(&tile_dev);
    pr_info("tile: removed\n");
}

module_init(tile_init);
module_exit(tile_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual Sentinel-2 tile device");
