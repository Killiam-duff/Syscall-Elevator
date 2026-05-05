Project Name: Elevator Kernel Module
(Use the main branch for the project submission)

## Group Members
  William Kilduff: wrk24@fsu.edu
  Kayla Callis: kmc23f@fsu.edu 
  Joshua Kim: jjk22g@fsu.edu 
## Division of Labor

### Part 1: Sytem Call Tracing
- **Responsibilities**: Create an empty C program. Make a copy and name it part1. Add five system calls to the part1 program. To verify that you have added the correct number of system calls, execute.
- **Assigned to**: William, Kayla, Joshua

### Part 2: Timer Kernel Module
- **Responsibilities**: The task requires creating a kernel module named my_timer that utilizes the function ktime_get_real_ts64() to retrieve the time value, which includes seconds and nanoseconds since the Epoch.
- **Assigned to**: William, Kayla, Joshua

### Part 3: Elevator Module
- **Responsibilities**: Your task is to implement a scheduling algorithm for an office elevator.
- **Assigned to**: William, Kayla, Joshua
- 3a:Adding System Calls.
-   Done by: William, Kayla, Joshua
- 3b:Kernel Compilation.
-   Done by: William, Kayla, Joshua 
- 3c:Threads.
-   Done by: William, Kayla, Joshua
- 3d:Linked List.
-   Done by: William, Kayla, Joshua
- 3e:Mutexes.
-   Done by: William, Kayla, Joshua
- 3f:Scheduling Algorithm.
-   Done by: William, Kayla, Joshua

## File Listing
```sh
project-2-group-10
│
├── part1/
│ ├── empty.c
│ ├── empty.trace
│ ├── part1.c
│ ├── part1.trace
│ ├── Makefile
│
├── part2/
│ ├── src/
|   ├── mytime.c
| ├── Makefile
│
├── part3/
|  ├── src/
|    ├── elevator.c
|  ├── Makefile
|  ├── syscall.c
|
├── README.md
└── Makefile
```
## How to Compile & Execute

### Requirements
- **Compiler**: GCC
- **Dependencies**:
<unistd.h>
<linux/init.h>
<linux/module.h>
<linux/proc_fs.h>
<linux/uaccess.h>
<linux/timekeeping.h>
<linux/slab.h>
<linux/string.h>
<linux/random.h>
<linux/list.h>
<linux/mutex.h>
<linux/kthread.h>
<linux/delay.h>
### Compilation
For a C/C++ example:
```
type "make"
```
This will make the kernel modules (for part 2/3. For part 1 follow the instructions on the project syllabus) 
### Execution
```
For part 1:
  $ gcc -o empty empty.c
  $ strace -o empty.trace ./empty
  $ gcc -o part1 part1.c
  $ strace -o part1.trace ./part1
For part 2:
  sudo insmod my_timer.ko
  cat /proc/timer
  sudo rmmod my_timer.ko
For part 3:
  sudo insmod elevator.ko
  $ watch -n [secs] cat [proc_file]
  ./producer [number_of_passengers]
  consumer --start/stop
  suod rmmod elevator.ko
```
This will run the program ...


## Bugs
1. Once elevator stop is called, the loading passenger in by calling producer returns 1. 
  
 

