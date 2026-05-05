#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Adds random passengers to a list and uses proc to query the stats");

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 2048
#define PERMS  0444
#define PARENT NULL

#define PASSENGER_PART 0
#define PASSENGER_LAWYER 1
#define PASSENGER_BOSS 2
#define PASSENGER_VISITOR 3

#define NUM_PASSENGER_TYPES 4
#define MAX_PASSENGER 5
#define MAX_WEIGHT 70
#define NUM_FLOORS 5

#define OFFLINE 0
#define IDLE 1
#define LOADING 2
#define UP 3
#define DOWN 4


//--structs--
//creating a linked list for passanger per floor
struct elevator {
        int state;
        int current_floor;
        int current_load;
        int passenger_count;
        int total_wait;
        int total_service;
	int stop_requested;

        struct list_head passenger;
        struct list_head floors[5];

        struct mutex lock;
};
//creates initial passanger linked list
struct passenger {
        int type;
        int weight;
        int start_floor;
        int destination_floor;
        struct list_head list;
};

static struct task_struct *elevator_threads;

static struct proc_ops fops;

static struct elevator elevator;
static char *message;
static int read_p;


//--Funtion Prototypes
void load_passenger(void);
void unload_passenger(void);
int add_passenger(int start_floor, int destination_floor, int type);
int print_passengers(void);
void delete_all_passenger(void);
int elevator_proc_open(struct inode *sp_inode, struct file *sp_file);
ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset);
int elevator_proc_release(struct inode *sp_inode, struct file *sp_file);
int elevator_thread_run(void *data);
int start_elevator(void);
int issue_request(int start_floor, int destination_floor, int type);
int stop_elevator(void);


/********************************************************************/
//function to load passengers onto the elevator
void load_passenger(void)
{	//linked list pointers
	struct list_head *temp, *dummy;
	struct passenger *p;	//passanger struct
	
	//iterates through each floor
	list_for_each_safe(temp, dummy, &elevator.floors[elevator.current_floor -1])
	{
		p = list_entry(temp, struct passenger, list);

		if (elevator.passenger_count >= MAX_PASSENGER)
			break;
		if (elevator.current_load + p->weight <= MAX_WEIGHT)
		{
			list_move_tail(temp, &elevator.passenger);

			elevator.current_load += p->weight;
			elevator.passenger_count++;
			elevator.total_wait--;
		}
	}
}

//uses pasenger struct to unload passangers from the floor lists
void unload_passenger(void)
{
	struct list_head *temp, *dummy;
	struct passenger *p;
	
	//linux kernel linked list macro for safe loading and unloading
	list_for_each_safe(temp, dummy, &elevator.passenger)
	{
		p = list_entry(temp, struct passenger, list);

		if (p->destination_floor==elevator.current_floor)
		{
			list_del(temp);

			elevator.current_load -= p->weight;
			elevator.passenger_count--;
			elevator.total_service++;
			kfree(p);
		}
	}	
}




int add_passenger(int start_floor, int destination_floor, int type) {
	struct passenger* p;
	int weight;

	
	if (start_floor < 1 || start_floor > 5 || destination_floor < 1 || destination_floor > 5 || start_floor==destination_floor)
		return -EINVAL;

	// setting weights for each passengers
	switch (type) {
		case PASSENGER_PART:
			weight = 10;
			break;
		case PASSENGER_LAWYER:
			weight = 15;
			break;
		case PASSENGER_BOSS:
			weight = 20;
			break;
		case PASSENGER_VISITOR:
			weight = 5;
			break;
		default:
			return -EINVAL;
	}
	
	mutex_lock(&elevator.lock);
	if(elevator.stop_requested)
	{
		mutex_unlock(&elevator.lock);
		return 1;
	}
	mutex_unlock(&elevator.lock);


	//need to build the list first since were adding from nothing
	//initializing the linked list
	p = kmalloc(sizeof(struct passenger) , GFP_KERNEL);
	if (p == NULL)
		return -ENOMEM;

	p->type = type;
	p->weight = weight;
	p->start_floor = start_floor;
	p->destination_floor = destination_floor;

	INIT_LIST_HEAD(&p->list);

	mutex_lock(&elevator.lock); //kthread?

	list_add_tail(&p->list, &elevator.floors[start_floor-1]); /* insert at back of list */
	elevator.total_wait++;

	mutex_unlock(&elevator.lock); //kthread?

	return 0;
}

int print_passengers(void) {
	struct passenger *p;
	struct list_head *temp;

	char *buf = kmalloc(sizeof(char) * 100, GFP_KERNEL);
	if (buf == NULL) {
		printk(KERN_WARNING "print_passengers");
		return -ENOMEM;
	}
	mutex_lock(&elevator.lock);
	/* init message buffer */
	strcpy(message, "");
	
	//gets the elevator state from the macro definition
	const char *current_elev_state[] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};
	
	//matches output from given elevator examples
	/* headers, print to temporary then append to message buffer */
	sprintf(buf, "Elevator state: %s", current_elev_state[elevator.state]);
	strcat(message, buf);
	sprintf(buf, "\nCurrent floor: %d", elevator.current_floor);
	strcat(message, buf);
	sprintf(buf, "\nCurrent load: %d lbs\n", elevator.current_load);
	strcat(message, buf);


	//Elevator status needs to go here (people in the elevator)

	strcat(message,"Elevator status:");
		list_for_each(temp, &elevator.passenger)
		{ /* forwards*/
			p = list_entry(temp, struct passenger, list);
			
			switch (p->type)
			{//still need to include the persons get off floor
			case PASSENGER_PART:
				sprintf(buf, "P%d " , p->destination_floor);
				strcat(message,buf);
				break;
			case PASSENGER_LAWYER:
				sprintf(buf, "L%d " , p->destination_floor);
				strcat(message,buf);
				break;
			case PASSENGER_BOSS:
				sprintf(buf, "B%d " , p->destination_floor);
				strcat(message,buf);
				break;
			case PASSENGER_VISITOR:
				sprintf(buf, "V%d " , p->destination_floor);
				strcat(message,buf);
			
				break;
			}

		}


		strcat(message, "\n\n");
	/* print entries */
	for (int i = 4; i >= 0; i--)
	{
		int count_floor = 0;
		//need to create a linked list on each floor
		list_for_each(temp,&elevator.floors[i])
			count_floor++;

		if (elevator.current_floor == i + 1)
			sprintf(buf, "[*] Floor %d: %d ", i+1,count_floor);
		else
			sprintf(buf, "[ ] Floor %d: %d ", i+1,count_floor);
		strcat(message, buf);

		list_for_each(temp, &elevator.floors[i])
		{ /* forwards*/
			p = list_entry(temp, struct passenger, list);
			
			switch (p->type)
			{//still need to include the persons get off floor
			case PASSENGER_PART:
				sprintf(buf, "P%d " , p->destination_floor);
				strcat(message,buf);
				break;
			case PASSENGER_LAWYER:
				sprintf(buf, "L%d " , p->destination_floor);
				strcat(message,buf);
				break;
			case PASSENGER_BOSS:
				sprintf(buf, "B%d " , p->destination_floor);
				strcat(message,buf);
				break;
			case PASSENGER_VISITOR:
				sprintf(buf, "V%d " , p->destination_floor);
				strcat(message,buf);
			
				break;
			}

		}
		
		strcat(message, "\n");
	}

	/* trailing newline to separate file from commands */
	strcat(message, "\n");

	sprintf(buf, "Number of passengers: %d", elevator.passenger_count);
	strcat(message, buf);
	sprintf(buf, "\nNumber of passengers waiting: %d", elevator.total_wait);
	strcat(message, buf);
	sprintf(buf, "\nNumber of passengers serviced: %d", elevator.total_service);
	strcat(message, buf);

	mutex_unlock(&elevator.lock);
	kfree(buf);
	return 0;
}

void delete_all_passenger(void) {
	struct list_head *temp;
	struct list_head *dummy;
	struct passenger *p;

	mutex_lock(&elevator.lock);
	//dynamically goes through the lists of list and deletes every passenger	
	list_for_each_safe(temp,dummy,&elevator.passenger)
	{
		p=list_entry(temp,struct passenger, list);
		list_del(temp);
		kfree(p);
	}
	for(int i=0; i<NUM_FLOORS; i++)
	{
		list_for_each_safe(temp,dummy,&elevator.floors[i])
		{
			p=list_entry(temp, struct passenger, list);
			list_del(temp);
			kfree(p);
		}
				

	}

	mutex_unlock(&elevator.lock);
}


/********************************************************************/
//proc file opener, this is from the professors examples
int elevator_proc_open(struct inode *sp_inode, struct file *sp_file) {
	read_p = 1;
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "elevator_proc_open");
		return -ENOMEM;
	}

	return print_passengers();
}

//same as proc_open
ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	int err_return;

	read_p = !read_p;
	if (read_p)
		return 0;

	err_return = copy_to_user(buf, message, len);
	if (err_return != 0)
		return -EFAULT;

	return len;
}

int elevator_proc_release(struct inode *sp_inode, struct file *sp_file) {
	kfree(message);
	return 0;
}

// KTHREAD

int elevator_thread_run(void *data)
{
        while (!kthread_should_stop()) 
	{
		
		//load or unload the passengers in, then a series of if statements that determines which
		//which direction the elevator moves
                mutex_lock(&elevator.lock);
		if(elevator.state ==OFFLINE)
		{
			mutex_unlock(&elevator.lock);
			ssleep(1);
			continue;
		}
		
		if(elevator.state==IDLE)
		{
			elevator.state=LOADING;
		}
		unload_passenger();	
		
		if(elevator.stop_requested && elevator.passenger_count==0 && elevator.total_wait==0)
		{
			elevator.state=OFFLINE;
			mutex_unlock(&elevator.lock);
			ssleep(2);
			continue;
		}

		if(!elevator.stop_requested)
			load_passenger();


		if(elevator.passenger_count==0 && elevator.total_wait==0)
		{
			elevator.state=IDLE;
		}
		else
		{
			if(elevator.state==LOADING)
				elevator.state=(elevator.current_floor==NUM_FLOORS) ? DOWN : UP;
		
			if(elevator.state==UP)
			{
				if(elevator.current_floor<NUM_FLOORS)
					elevator.current_floor++;
				else
					elevator.state=DOWN;
			}
			else if(elevator.state==DOWN)
			{

				if(elevator.current_floor>1)
					elevator.current_floor--;
				else
					elevator.state=UP;
			}
		}
		mutex_unlock(&elevator.lock);
		
		ssleep(2);
		
	}

        return 0;
}

/********************************************************************/


int start_elevator(void)
{
	mutex_lock(&elevator.lock);
	
	//if elevator is already started then there is an error
	if (elevator.state != OFFLINE){
		mutex_unlock(&elevator.lock);
		return 1;
	}

	elevator.state = IDLE;
	elevator.stop_requested=0;

	mutex_unlock(&elevator.lock);
	
	return 0;
}
EXPORT_SYMBOL(start_elevator);

int issue_request(int start_floor, int destination_floor, int type)
{
	return add_passenger(start_floor, destination_floor, type);
}
EXPORT_SYMBOL(issue_request);

int stop_elevator(void)
{	
	mutex_lock(&elevator.lock);
	if(elevator.state==OFFLINE)
	{
		mutex_unlock(&elevator.lock);
		return 1;
	}
	elevator.stop_requested=1;
	elevator.state = 0;
	elevator.current_load = 0;
	elevator.passenger_count = 0;
	elevator.total_wait = 0;
	elevator.total_service = 0;

	//must iterate through and delete all passengers
	struct list_head *temp; //current node
	struct list_head *safe; // pointer to next node
        struct passenger *p;
	list_for_each_safe(temp, safe, &elevator.passenger) {
		p = list_entry(temp, struct passenger, list);
		list_del(temp);
		kfree(p);
	}
	//remove the passengers waiting in line on each floor
	for (int i = 0; i < 5; i++) {
		list_for_each_safe(temp, safe, &elevator.floors[i]){
			p = list_entry(temp, struct passenger, list);
	       		list_del(temp);
	 		kfree(p);
		}	
	}

	mutex_unlock(&elevator.lock);
	
	return 0;
}
EXPORT_SYMBOL(stop_elevator);

static int elevator_init(void) {
	fops.proc_open = elevator_proc_open;
	fops.proc_read = elevator_proc_read;
	fops.proc_release = elevator_proc_release;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "elevator_init\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
	
	elevator.state = 0; //STATE_OFFLINE;
	elevator.current_floor = 1;
	elevator.current_load = 0;
	elevator.passenger_count = 0;
	elevator.total_wait = 0;
	elevator.total_service = 0;
	elevator.stop_requested=0;


	//initializing the floors
	mutex_init(&elevator.lock);
	INIT_LIST_HEAD(&elevator.passenger);
	//increment trhough each floor
	for (int i = 0; i < 5; i++)	
		INIT_LIST_HEAD(&elevator.floors[i]);

	//elevator_threads = NULL;
	
	//calls on the created syscalls
	STUB_start_elevator = start_elevator;
	STUB_issue_request = issue_request;
	STUB_stop_elevator = stop_elevator;
	
	elevator_threads = kthread_run(elevator_thread_run,NULL,"elevator_thread");	

	return 0;
}
module_init(elevator_init);


static void elevator_exit(void) {
	if(elevator_threads)
		kthread_stop(elevator_threads);

	delete_all_passenger();
	remove_proc_entry(ENTRY_NAME, NULL);
}
module_exit(elevator_exit);
