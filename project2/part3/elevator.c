#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
MODULE_LICENSE("GPL");
#define MAX_WEIGHT 10
#define MAX_PASS_UNITS 8
#define MAX_FLOOR 10
#define MIN_FLOOR 1
#define LOAD_WAIT 1
#define FLOOR_CHANGE_TIME 2
#define ENTRY_NAME "elevator"
#define PERMS 0644
extern int (*STUB_issue_request)(int a, int b, int c);
extern int (*STUB_start_elevator)(void);
extern int (*STUB_stop_elevator)(void);

//creates the structure to hold people while waiting
struct person **floors;
//stores how many people are waiting per floor
int * num_people;
static int read_p;
static char *message;
static struct file_operations fops;
int i;

//structure to be passed to kthread_run
struct thread_parameter {
	int id;
	struct task_struct *kthread;
	struct mutex my_mutex;
};

struct thread_parameter thread1;

struct elevator{
     int floor;
     int next_floor;
     enum {IDLE, OFFLINE, LOADING, UP, DOWN} state;
     struct person* people;
     int num_people;
     int pass_units;
     int weight_units;
};
struct person{
     int weight;
     int passenger;
     int destination_floor;
     //enum {ADULT=0, CHILD=1, ROOM_SERVICE=2, BELL_HOP=3} title;
};

     enum {ADULT=0, CHILD=1, ROOM_SERVICE=2, BELL_HOP=3};

     bool active = false;
     bool stopping = false;
     struct elevator elev;
     
bool no_one_waiting(void){
     int tmp;
     for(tmp = 0; tmp < 10; tmp++){
         if(num_people[tmp] != 0){
	    return false;
	 }
     }
     return true;
}

void remove_person_elevator(int position){
     int tmp;
     //i is for counting up through all people except the one to remove
     int i;
     struct person* tmp_person = (struct person *)vmalloc((elev.num_people-1)*sizeof(struct person));
     //remove the passengers weight
     //TODO may need to check that .passenger is what i think it is
     elev.weight_units -= elev.people[position].weight;
     elev.pass_units -= elev.people[position].passenger;
     for(tmp = 0; tmp < elev.num_people; tmp++){
	 //copy everyone but the person to remove
         if(tmp!=position){
	     tmp_person[i].weight = elev.people[tmp].weight;
	     tmp_person[i].passenger = elev.people[tmp].passenger;
	     tmp_person[i].destination_floor = elev.people[tmp].destination_floor;
             i++;
	 }
     }
     vfree(elev.people);
     elev.people = tmp_person;
     //take off the person from the number of people on it counter
     elev.num_people--;
}

//takes the enumeration for which person is which and which floor to take from
void add_person_elevator(int person, int floor){
    int i;
    for(i = 0; i < num_people[floor]; i++){
        //if()
    }
}
//TODO make this function unload then load people and return true if anyone gets on or off
bool passenger_check(void){
     int tmp;
     bool load_unload = false;
     //for unloading people from the elevator
     for(tmp = 0; tmp < elev.num_people; tmp++){
         if(elev.people[tmp].destination_floor == elev.floor){
	    remove_person_elevator(tmp);
	    //remove the person and decrement tmp so that it will recheck this persons position and not skip over a potential nother person wanting to get off
	    tmp--; 
	 }
     }

     //for putting people in the elevator
     for(tmp = 0; tmp < num_people[elev.floor]; tmp++){
         
     }
     return load_unload;
}

void move(void){
     //current implementation ideas
     //the elevator goes to which ever floor people want or are waiting at in the current direction of travel until there are no more requests in that direction. Then check the other direction and if no one is found just idle
     int floor_difference;
     int tmp;
     floor_difference = elev.next_floor - elev.floor;
     //moving up
     if(floor_difference > 0){
          elev.floor++;
	  //check how many people want to get off at this floor then
	  //check how many people want to get on at this floor and are able to
	  //in this order the people will exit first before they enter
	  if(passenger_check()){
              //sleep for 1 second since it takes that long to load/unload people
	      ssleep(1);
	  }
     //moving down
     }else if(floor_difference < 0){
          elev.floor--;
	  if(passenger_check()){
	     ssleep(1);
	  }
     }else{
     //this is if the elevator is on the final destination floor
     }
}


int elevator_proc_open(struct inode *sp_inode, struct file *sp_file){
     printk(KERN_INFO "Trying to open proc/timed\n");
     read_p = 1;
     message = kmalloc(sizeof(char)*1000, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
     if (message == NULL){
         printk(KERN_WARNING "my_xtimed couldn't allocate any memory for messages\n");
         return -ENOMEM;
     }
     switch(elev.state){
	     case OFFLINE : sprintf(message,"State: OFFLINE\n");
			    break;
	     case IDLE : sprintf(message, "State: IDLE\n");
			 break;
	     case LOADING : sprintf(message, "State: LOADING\n");
			    break;
	     case UP : sprintf(message, "State: UP\n");
	       		break;
	     case DOWN : sprintf(message, "State: DOWN\n");
	                break;
	     default: sprintf(message, "Error determining elevator state\n");
     			break;		      
     }
     return 0;

}

ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
        int len = strlen(message);

	read_p = !read_p;
	if(read_p){
		return 0;
	}
        printk(KERN_INFO "proc called read\n");
        copy_to_user(buf, message, len);
        return len;
}

int elevator_proc_release(struct inode *sp_inode, struct file *sp_file) {
        printk(KERN_NOTICE "proc called release\n");
        kfree(message);
        return 0;
}

int move_that_elevator(void *data){
	while(!kthread_should_stop() || stopping){
		ssleep(2);
		if(mutex_lock_interruptible(&thread1.my_mutex) == 0){
			printk(KERN_ALERT "thread works fool ya fool\n");
		}
		//if the elevator has been requested to stop check that it isnt done yet
		if(stopping){
			if(elev.num_people == 0 && no_one_waiting()){
			          stopping = false;
				  elev.state = IDLE;
			}
		}
		move();
		mutex_unlock(&thread1.my_mutex);
	}
	return 1;
}

int start_elevator(void){
     //check that elevator isnt already active
     if(active==true){
        return 1;
     }else{
	//allocating pointer of pointers for floors
	floors = (struct person**)vmalloc(MAX_FLOOR*sizeof(struct person*));
	if(floors == NULL){
		printk(KERN_ALERT "memory allocation error\n");
	   return -ENOMEM;
	}
	//initializing the array holding how many people are waiting to 0 and floors[0-9] to null
	num_people = (int*)vmalloc(MAX_FLOOR*sizeof(int));
	for(i = 0; i < MAX_FLOOR; i++){
		floors[i] = NULL;
		num_people[i] = 0;
	}
	//initializing the elev.people array to be able to hold 8 people since thats the max with the current weight and size limit
	elev.people = (struct person*)vmalloc(8*sizeof(struct person));
	if(elev.people == NULL){
		printk(KERN_ALERT "memory allocation error\n");
	   return -ENOMEM;
	}
	if(floors == NULL || num_people == NULL || elev.people ==NULL){
		printk(KERN_ALERT "memory allocation error\n");
	}
	//setting elevator starting states
	elev.state = IDLE;
	elev.floor = 1;
	elev.pass_units = 0;
	elev.weight_units = 0;
	active = true;

	//starting the thread for the moving of the elevator
	thread1.kthread = kthread_run(move_that_elevator, &thread1,"moving elevator thread thing");	
	//TODO make sure thread starts if not explode
	
	//initilizing the mutex so that it will actuall work
	mutex_init(&thread1.my_mutex);
        return 0;
     }
}


//a is passenger type
//b is starting floor
//c is destination floor
int issue_request(int a, int b, int c){
	struct person* tmp = NULL;
	//allocated memory goes from 0-9 so shifting floors to match
	b--;
	c--;
	if(mutex_lock_interruptible(&thread1.my_mutex) == 0){
		for(i = 0; i < MAX_FLOOR; i++){
		//	printk(KERN_ALERT "Floor [%d] \n", i);
		//	printk(KERN_ALERT "People waiting [%d] \n", num_people[i]);
		}
		//allocating remove for one more person on the floor and copying all over to a new array then destroying original
		tmp = (struct person*)vmalloc((num_people[b]+1)*sizeof(struct person));
        	if(tmp == NULL){
		   printk(KERN_ALERT "memory allocation error\n");
        	   return -ENOMEM;
        	}
		for(i = 0; i < num_people[b]; i++){
			tmp[i].weight = floors[b][i].weight;
			tmp[i].passenger = floors[b][i].passenger;
			tmp[i].destination_floor = floors[b][i].destination_floor;
		//	tmp[i].title = floors[b][i].title;
		}
		if(a == ADULT){
			tmp[i+1].weight = 1;
			tmp[i+1].passenger = 1;
			tmp[i+1].destination_floor = c;
		//	tmp[i+1].title = a;
		}else if(a == CHILD){
			//TODO CHILD WEIGHT IS WRONG SHOULD BE 1/2
			tmp[i+1].weight = 0;
			tmp[i+1].passenger = 1;
			tmp[i+1].destination_floor = c;
		//	tmp[i+1].title = a;
		}else if(a == ROOM_SERVICE){
			tmp[i+1].weight = 2;
			tmp[i+1].passenger = 1;
			tmp[i+1].destination_floor = c;
		//	tmp[i+1].title = a;
		}else if(a == BELL_HOP){
			tmp[i+1].weight = 2;
			tmp[i+1].passenger = 2;
			tmp[i+1].destination_floor = c;
		//	tmp[i+1].title = a;
		}
		num_people[b]++;
		if(floors[b]!=NULL){
			vfree(floors[b]);
        	}	
		floors[b] = tmp;
		}
		mutex_unlock(&thread1.my_mutex);
	return 0;
}

int stop_elevator(void){
        stopping = true;
	active = false;
	return 0;
}

static int __init elevator_init(void){
     //started elevator states
     active = false;
     stopping = false;
     elev.state = OFFLINE;
     elev.num_people = 0;
     STUB_start_elevator = &start_elevator;
     STUB_stop_elevator = &stop_elevator;
     STUB_issue_request = &issue_request;
        fops.open = elevator_proc_open;
        fops.read = elevator_proc_read;
        fops.release = elevator_proc_release;

       if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
               printk(KERN_WARNING "proc create\n");
               remove_proc_entry(ENTRY_NAME, NULL);
               return -ENOMEM;
       }

     return 0;
}
static void __exit elevator_exit(void){
     //removing proc entry
     remove_proc_entry(ENTRY_NAME, NULL);
     printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
     printk(KERN_ALERT "End of elevator\n");
     //requesting thread to stop
     int ret = kthread_stop(thread1.kthread);
     if(ret != -EINTR){
         printk("the fool has been silenced\n");
     }
     mutex_destroy(&thread1.my_mutex);
     STUB_start_elevator = NULL;
     STUB_stop_elevator = NULL;
     STUB_issue_request = NULL;
     //freeing memory
     for(i = 0; i < 10; i++){
     	vfree(floors[i]);
	if(floors[i]==NULL){
	   printk("YOOOO floor %d is good\n",i);
	}
     }
     vfree(floors);
}



module_init(elevator_init);
module_exit(elevator_exit);
