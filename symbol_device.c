#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vasiliy");
MODULE_DESCRIPTION("A simple character device");

/* Количество устройств, с которыми ведется работа. */
#define MINOR_DEVICES   1

/* Название устройства. */
#define DEVICE_NAME     "symbol_device"

/* Структура драйвера. */
struct dev_drv {
    dev_t id;
    struct cdev chardev;
    struct class *class;
    struct device *device;
};

/* Экземпляр структуры драйвера. */
static struct dev_drv device_drv;

/* Чтение из ядра. */
static ssize_t module_read(struct file *f, char *buf, size_t size, loff_t *offset) {
    buf = "test message";
    printk(KERN_INFO "%s: read from module: %s\n", DEVICE_NAME, buf);
    return 0;
}

/* Запись в ядро. */
static ssize_t module_write(struct file *f, const char *buf, size_t size, loff_t *offset) {
    printk(KERN_INFO "%s: write to module: %s\n", DEVICE_NAME, buf);
    return 0;
}

/* Открытие файла. */
static int module_open(struct inode *inod, struct file *f) {
    printk(KERN_INFO "%s: module is opened\n", DEVICE_NAME);
    return 0;
}

/* Закрытие файла. */
static int module_release(struct inode *inod, struct file *f) {
    printk(KERN_INFO "%s: module is released\n", DEVICE_NAME);
    return 0;
}

/* Структура с хуками файловых операций. */
static const struct file_operations fops = {
    .read = module_read,
    .write = module_write,
    .open = module_open,
    .release = module_release
};
 
static int __init test_init(void) {
    int ret = 0;

    /* Register a range of char device numbers. */
    ret = alloc_chrdev_region(&(device_drv.id), 0, MINOR_DEVICES, DEVICE_NAME);
    if (ret != 0) {
        printk(KERN_ERR "%s: alloc_chrdev_region failed (%d)\n", DEVICE_NAME, ret);
        return ret;
    }

    /* Create a struct class structure. */
    device_drv.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(device_drv.class)) {
        printk(KERN_ERR "%s: class_create failed\n", DEVICE_NAME);
        unregister_chrdev_region(device_drv.id, MINOR_DEVICES);
        device_drv.id = 0;
        return -ENOMEM;
    }

    /* Initialize a cdev structure. */
    cdev_init(&(device_drv.chardev), &fops);

    /* Add a char device to the system. */
    ret = cdev_add(&(device_drv.chardev), MKDEV(MAJOR(device_drv.id), MINOR(device_drv.id)), 1);

    /* Creates a device and registers it with sysfs. */
    device_drv.device = device_create(device_drv.class, NULL, MKDEV(MAJOR(device_drv.id), MINOR(device_drv.id)), (void *) &device_drv, DEVICE_NAME);
    if (IS_ERR(device_drv.device)) {
        printk(KERN_ERR "%s: device_create failed\n", DEVICE_NAME);
        cdev_del(&(device_drv.chardev));
        unregister_chrdev_region(device_drv.id, MINOR_DEVICES);
        device_drv.id = 0;
        return -ENODEV;
    }

	printk(KERN_INFO "%s: module is started\n", DEVICE_NAME);
    return 0;
}

static void __exit test_exit(void) {
    device_destroy(device_drv.class, MKDEV(MAJOR(device_drv.id), MINOR(device_drv.id)));
    class_destroy(device_drv.class);

    cdev_del(&(device_drv.chardev));

    unregister_chrdev_region(device_drv.id, MINOR_DEVICES);
    device_drv.id = 0;

    printk(KERN_INFO "%s: module is finished\n", DEVICE_NAME);
}

module_init(test_init);
module_exit(test_exit);
 