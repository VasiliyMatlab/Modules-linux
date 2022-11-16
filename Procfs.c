#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vasiliy");
MODULE_DESCRIPTION("An example of usage /proc file system");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
    #define HAVE_PROC_OPS
#endif

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "kern_process"

/* Эта структура содержит информацию о файле /proc. */
static struct proc_dir_entry *our_proc_file;

/* Этот буфер используется под хранение символа для данного модуля. */
static char procfs_buffer[PROCFS_MAX_SIZE];

/* Размер буфера. */
static unsigned long procfs_buffer_size = 0;

/* Эта функция вызывается при считывании файла /proc. */ 
static ssize_t procfile_read(struct file *filePointer, char __user *buffer, size_t buffer_length, loff_t *offset) {
    static int finished = 0;

    if (finished) {
        printk(KERN_DEBUG "%s: procfs_read: END\n", PROCFS_NAME);
        finished = 0;
        return 0;
    }
    finished = 1;

    if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size))
        return -EFAULT;

    printk(KERN_DEBUG "%s: procfs_read: read %lu bytes\n", PROCFS_NAME, procfs_buffer_size);
    return procfs_buffer_size;
}

/* Эта функция вызывается при записи файла /proc. */
static ssize_t procfile_write(struct file *file, const char __user *buff, size_t len, loff_t *off) {
    if (len > PROCFS_MAX_SIZE)
        procfs_buffer_size = PROCFS_MAX_SIZE;
    else
        procfs_buffer_size = len;
    if (copy_from_user(procfs_buffer, buff, procfs_buffer_size))
        return -EFAULT;

    printk(KERN_DEBUG "%s: procfs_write: write %lu bytes\n", PROCFS_NAME, procfs_buffer_size);
    return procfs_buffer_size;
}

/* Эта функция вызывается при открытии файла. */
static int procfs_open(struct inode *inode, struct file *file) {
    try_module_get(THIS_MODULE);
    return 0;
}

/* Эта функция вызывается при закрытии файла. */
static int procfs_close(struct inode *inode, struct file *file) {
    module_put(THIS_MODULE);
    return 0;
}

#ifdef HAVE_PROC_OPS
    static const struct proc_ops proc_file_fops = {
        .proc_read = procfile_read,
        .proc_write = procfile_write,
        .proc_open = procfs_open,
        .proc_release = procfs_close
    };
#else
    static const struct file_operations proc_file_fops = {
        .read = procfile_read,
        .write = procfile_write,
        .open = procfs_open,
        .release = procfs_close
    };
#endif

static int __init procfs_init(void) {
    our_proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_file_fops);
    if (our_proc_file == NULL) {
        remove_proc_entry(PROCFS_NAME, NULL);
        printk(KERN_DEBUG "%s: error: Could not initialize /proc/%s\n", PROCFS_NAME, PROCFS_NAME);
        return -ENOMEM;
    }

    proc_set_size(our_proc_file, 80);
    proc_set_user(our_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);

    printk(KERN_DEBUG "%s: /proc/%s created\n", PROCFS_NAME, PROCFS_NAME);
    return 0;
}

static void __exit procfs_exit(void) {
    remove_proc_entry(PROCFS_NAME, NULL); 
    printk(KERN_DEBUG "%s: /proc/%s removed\n", PROCFS_NAME, PROCFS_NAME); 
}

module_init(procfs_init);
module_exit(procfs_exit);
