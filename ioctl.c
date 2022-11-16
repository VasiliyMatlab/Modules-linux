#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vasiliy");
MODULE_DESCRIPTION("An example of use ioctl");

/* Формат аргумента ioctl. */
struct ioctl_arg {
    unsigned int val;
};

/* Данные ioctl. */
struct ioctl_data {
    unsigned char val;
    rwlock_t lock;
};

/* Уникальный номер для ioctl. */
#define IOC_MAGIC   'i'

/* Записать аргумент в ядро. */
#define IOCTL_VALSET _IOW(IOC_MAGIC, 0, struct ioctl_arg)
/* Получить аргумент из ядра. */
#define IOCTL_VALGET _IOR(IOC_MAGIC, 1, struct ioctl_arg)
/* Записать число в ядро. */
#define IOCTL_VALSET_NUM _IOW(IOC_MAGIC, 2, int)
/* Получить число из ядра. */
#define IOCTL_VALGET_NUM _IOR(IOC_MAGIC, 3, int)

/* Название драйвера. */
#define DRIVER_NAME "ioctltest"

/* Мажорный номер. */
static unsigned int test_ioctl_major = 0;
/* Количество устройств. */
static unsigned int num_of_dev = 1;
/* Символьное устройство. */
static struct cdev test_ioctl_cdev;
/* Структура класса. */
static struct class *cls;
/* Локальная переменная модуля. */
static int ioctl_num = 0;

/* Обработка ioctl. */
static long ioctltest_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    struct ioctl_data *test_data = f->private_data;
    int ret = 0;
    unsigned char val;
    struct ioctl_arg arg_data = {0};
    memset(&arg_data, 0, sizeof(arg_data));

    printk(KERN_INFO "%s: %s call\n", DRIVER_NAME, __func__);

    switch (cmd) {
    case IOCTL_VALSET:
        if (copy_from_user(&arg_data, (int __user *) arg, sizeof(arg_data))) {
            ret = -EFAULT;
            break;
        }

        printk(KERN_DEBUG "%s: ioctl set val: %x\n", DRIVER_NAME, arg_data.val);
        write_lock(&test_data->lock);
        test_data->val = arg_data.val;
        write_unlock(&test_data->lock);
        break;
    case IOCTL_VALGET:
        read_lock(&test_data->lock);
        val = test_data->val;
        read_unlock(&test_data->lock);
        arg_data.val = val;

        if (copy_to_user((int __user *) arg, &arg_data, sizeof(arg_data))) {
            ret = -EFAULT;
        }
        break;
    case IOCTL_VALSET_NUM:
        ioctl_num = arg;
        break;
    case IOCTL_VALGET_NUM:
        ret = __put_user(ioctl_num, (int __user *) arg);
        break;
    default:
        ret = ENOTTY;
        break;
    }

    return ret;
}

static ssize_t ioctltest_read(struct file *f, char __user *buf, size_t count, loff_t *f_pos) {
    struct ioctl_data *test_data = f->private_data;
    unsigned char val;
    static unsigned char flag = 0;

    printk(KERN_INFO "%s: %s call\n", DRIVER_NAME, __func__);

    if (flag) {
        flag = 0;
        return 0;
    }
    flag = 1;

    read_lock(&test_data->lock);
    val = test_data->val;
    read_unlock(&test_data->lock);
    printk(KERN_DEBUG "%s: val = %u\n", DRIVER_NAME, val);

    if (copy_to_user(&buf[0], &val, 1)) {
        return -EFAULT;
    }

    return count;
}

static ssize_t ioctltest_write(struct file *f, const char __user *buf, size_t count, loff_t *f_pos) {
    struct ioctl_data *test_data = f->private_data;
    char tmp[8];

    printk(KERN_INFO "%s: %s call\n", DRIVER_NAME, __func__);

    if (copy_from_user(tmp, (int __user *) buf, sizeof(tmp))) {
        return -EFAULT;
    }
    write_lock(&test_data->lock);
    test_data->val = tmp[0];
    write_unlock(&test_data->lock);

    printk(KERN_DEBUG "%s: test_data->val = %u\n", DRIVER_NAME, test_data->val);
    printk(KERN_DEBUG "%s: tmp[0] = %d\n", DRIVER_NAME, tmp[0]);

    return 0;
}

static int ioctltest_open(struct inode *inode, struct file *f) {
    struct ioctl_data *test_data;

    printk(KERN_INFO "%s: %s call\n", DRIVER_NAME, __func__);
    test_data = kzalloc(sizeof(struct ioctl_data), GFP_KERNEL);
    if (test_data == NULL) {
        printk(KERN_ERR "%s: kzalloc failed\n", DRIVER_NAME);
        return -ENOMEM;
    }

    rwlock_init(&test_data->lock);
    test_data->val = 105;
    f->private_data = test_data;

    return 0;
}

static int ioctltest_close(struct inode *inode, struct file *f) {
    printk(KERN_INFO "%s: %s call\n", DRIVER_NAME, __func__);

    if (f->private_data) {
        kfree(f->private_data);
        f->private_data = NULL;
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ioctltest_open,
    .release = ioctltest_close,
    .read = ioctltest_read,
    .write = ioctltest_write,
    .unlocked_ioctl = ioctltest_ioctl
};

static int __init ioctl_init(void) {
    dev_t dev;
    int ret = 0;

    ret = alloc_chrdev_region(&dev, 0, num_of_dev, DRIVER_NAME);
    if (ret) {
        printk(KERN_ERR "%s: alloc_chrdev_region failed\n", DRIVER_NAME);
        return ret;
    }

    test_ioctl_major = MAJOR(dev);

    cls = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(cls)) {
        printk(KERN_ERR "%s: class_create failed\n", DRIVER_NAME);
        unregister_chrdev_region(dev, num_of_dev);
        return -ENOMEM;
    }

    device_create(cls, NULL, dev, NULL, DRIVER_NAME);

    cdev_init(&test_ioctl_cdev, &fops);

    ret = cdev_add(&test_ioctl_cdev, dev, num_of_dev);
    if (ret) {
        printk(KERN_ERR "%s: cdev_add failed\n", DRIVER_NAME);
        device_destroy(cls, dev);
        class_destroy(cls);
        unregister_chrdev_region(dev, num_of_dev);
        return ret;
    }

    printk(KERN_INFO "%s: %s driver(major: %d) installed\n", DRIVER_NAME, DRIVER_NAME, test_ioctl_major);
    return ret;
}

static void __exit ioctl_exit(void) {
    dev_t dev = MKDEV(test_ioctl_major, 0);

    cdev_del(&test_ioctl_cdev);
    device_destroy(cls, dev);
    class_destroy(cls);
    unregister_chrdev_region(dev, num_of_dev);
    printk(KERN_INFO "%s: %s driver removed\n", DRIVER_NAME, DRIVER_NAME);
}

module_init(ioctl_init);
module_exit(ioctl_exit);
