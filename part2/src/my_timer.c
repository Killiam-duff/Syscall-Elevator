#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("kernel module for timer");

#define ENTRY_NAME "timer"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* timer_entry;
//static int first_time = 1;
//static struct timespec64 start_time;
static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    static struct timespec64 start_time;
    static int init = 0;

    struct timespec64 ts_now;
    struct timespec64 elapsed;
    char buf[256];
    int len = 0;

    ktime_get_real_ts64(&ts_now);

    if (*ppos > 0)
	return 0;

    if(!init)
    {
	start_time = ts_now;
	init = 1;
	len = snprintf(buf, sizeof(buf), "current time: %lld.%.09ld\n", (long long)ts_now.tv_sec, ts_now.tv_nsec);
  
        return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
    }


    elapsed.tv_sec = ts_now.tv_sec - start_time.tv_sec;
    elapsed.tv_nsec = ts_now.tv_nsec - start_time.tv_nsec;

    if(elapsed.tv_nsec < 0)
    {
	elapsed.tv_sec--;
	elapsed.tv_nsec *=-1;
    }

    len = snprintf(buf, sizeof(buf), "current time: %lld.%.09ld\nelapsed time: %lld.%.09ld\n", 
		(long long)ts_now.tv_sec, ts_now.tv_nsec, (long long)elapsed.tv_sec, elapsed.tv_nsec);
    start_time = ts_now; 
    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
    
}

static const struct proc_ops timer_fops = {
    .proc_read = timer_read,
};

static int __init timer_init(void)
{
    timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    if (!timer_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);
