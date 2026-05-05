#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

// NOTE: ** this is just a demonstration of a system call, you should directly add the 3 needed calls not these unnecessary call!

// sys call stubs
int (*STUB_start_elevator)(void) = NULL;
int (*STUB_issue_request)(int, int, int) = NULL;
int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

// sys call wrappers
SYSCALL_DEFINE0(start_elevator) {    // change this to your system call name
    printk(KERN_NOTICE "Inside SYSCALL_DEFINE0 block. %s", __FUNCTION__);
    if(STUB_start_elevator != NULL)
        return STUB_start_elevator();
    else
        return -ENOSYS;
}

SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type) { //our system call name and parameters.
    printk(KERN_NOTICE "Inside SYSCALL_DEFINE3 block. %s: Your int are %d, %d\n", __FUNCTION__, start_floor, destination_floor);
    if(STUB_issue_request != NULL)
        return STUB_issue_request(start_floor, destination_floor, type);
    else
        return -ENOSYS;
}

// sys call wrappers
SYSCALL_DEFINE0(stop_elevator) {    // change this to your system call name
    printk(KERN_NOTICE "Inside SYSCALL_DEFINE0 block. %s", __FUNCTION__);
    if(STUB_stop_elevator != NULL)
        return STUB_stop_elevator();
    else
        return -ENOSYS;
}

