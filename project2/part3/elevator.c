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
int* number_serviced;

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
     i = 0;
     //remove the passengers weight
     //TODO may need to check that .passenger is what i think it is
     elev.weight_units -= elev.people[position].weight;
     elev.pass_units -= elev.people[position].passenger;
     if(elev.num_people-1>0){
     struct person* tmp_person = (struct person *)vmalloc((elev.num_people-1)*sizeof(struct person));
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
     }
     //take off the person from the number of people on it counter
     elev.num_people--;
}

//takes the enumeration for which person is which and which floor to take from
bool add_person_elevator(int person, int floor){
    int i;
    int j;
    //allocating memory for the swap
    struct person* for_elevator;
    struct person* for_floor;
    for_floor = NULL;
    for_elevator = NULL;

    //printk("testing %d %d\n",num_people[floor],elev.num_people);
        if(floors[floor][person].weight+elev.weight_units<=20 && floors[floor][person].passenger+elev.pass_units<=8 && !stopping){
	   elev.num_people++;
	   num_people[floor]--;
	   elev.weight_units += floors[floor][person].weight;
	   elev.pass_units += floors[floor][person].passenger;
	   for_elevator = (struct person*)vmalloc(elev.num_people*sizeof(struct person));
           if(for_elevator == NULL){
               printk(KERN_WARNING "Elevator couldn't allocate any memory for messages\n");
               return -ENOMEM;
	   }
	   if(num_people[floor]>0){
           for_floor = (struct person*)vmalloc(num_people[floor]*sizeof(struct person));
	   if(for_floor == NULL){
               printk(KERN_WARNING "Elevator couldn't allocate any memory for messages\n");
               return -ENOMEM;
	   }
	   }
	   //commencing the swap
	   for(i = 0; i < elev.num_people-1; i++){
	       for_elevator[i].weight = elev.people[i].weight;
	       for_elevator[i].passenger = elev.people[i].passenger;
	       for_elevator[i].destination_floor = elev.people[i].destination_floor;
	   }
	   if(elev.num_people>0){
	       for_elevator[elev.num_people-1].weight = floors[floor][person].weight;
	       for_elevator[elev.num_people-1].passenger = floors[floor][person].passenger;
	       for_elevator[elev.num_people-1].destination_floor = floors[floor][person].destination_floor;
	   }
	   //going over the full number of people since the individual hasnt technically been removed yet
	   j = 0;
	   for(i = 0; i <= num_people[floor]; i++){
		   
	       if(i != person){
	          for_floor[j].weight = floors[floor][i].weight;
	          for_floor[j].destination_floor = floors[floor][i].destination_floor;
	          for_floor[j].passenger = floors[floor][i].passenger;
		  j++;
	 	}
	   }
	   if(num_people[floor]>0){
	      vfree(floors[floor]);
	      floors[floor]=for_floor;
	   }
	   vfree(elev.people);
	   elev.people=for_elevator;
           return true;	
	}else{
	   return false;
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
            number_serviced[elev.floor]++;
	    load_unload = true;
	    //remove the person and decrement tmp so that it will recheck this persons position and not skip over a potential nother person wanting to get off
	    tmp = 0; 
	 }
     }

     //for putting people in the elevator
     //printk("testing peop on floor %d\n",num_people[elev.floor]);
     for(tmp = 0; tmp < num_people[elev.floor]; tmp++){
		 //TODO make it so people are only added in the direction of current travel or if the destination = current floor
	         if(add_person_elevator(tmp,elev.floor)){
		    load_unload = true;
		    tmp--;
		    //already did this in the add_person_elevator
		    //num_people[elev.floor]--;
		 }
	     //if(person gets on){
	     //   load_unload = true;
	     //}
         
     }
     return load_unload;
}

void move(void){
     int i;
     int j;
     //current implementation ideas
     //the elevator goes to which ever floor people want or are waiting at in the current direction of travel until there are no more requests in that direction. Then check the other direction and if no one is found just idle
     int floor_difference;
     //upwards and downwards are used to dermine the magnitude of the distance of moving in either the upwards or downwards direction if the elevator currently doesnt know which way to go aka the else condition
     int upwards;
     int downwards;
     int upwards_destination;
     int downwards_destination;
     floor_difference = elev.next_floor - elev.floor;
     //moving up
     if(floor_difference > 0){
	  elev.state = UP;
          elev.floor++;
	  //check how many people want to get off at this floor then
	  //check how many people want to get on at this floor and are able to
	  //in this order the people will exit first before they enter
	  if(passenger_check()){
              //sleep for 1 second since it takes that long to load/unload people
	      elev.state = LOADING;
	      ssleep(1);
	      elev.state = UP;
	  }
     //moving down
     }else if(floor_difference < 0){
          elev.floor--;
	  elev.state = DOWN;
	  if(passenger_check()){
             elev.state = LOADING;
	     ssleep(1);
	     elev.state = DOWN;
	  }
     }else{
     //this is if the elevator is on the final destination floor
         if(passenger_check()){
            elev.state = LOADING;
	    ssleep(1);
	 }
	 if(elev.num_people>0){
         upwards = 0;
         downwards = 0;
	 upwards_destination = elev.floor;
	 downwards_destination = elev.floor;
             for(i = 0; i < elev.num_people; i++){
                 if(elev.people[i].destination_floor>elev.floor){
		     if(elev.people[i].destination_floor-elev.floor>upwards){
		         upwards = elev.people[i].destination_floor-elev.floor;
			 upwards_destination = elev.people[i].destination_floor;
		     } 
		 }
             }
             for(i = 0; i < elev.num_people; i++){
                 if(elev.people[i].destination_floor<elev.floor){
		     if(elev.floor-elev.people[i].destination_floor>downwards){
		         downwards = elev.floor-elev.people[i].destination_floor;
			 downwards_destination = elev.people[i].destination_floor;
		     } 
		 }
             }
	     if(upwards>=downwards){
	         elev.next_floor = upwards_destination;
		 elev.state = UP;
	     }else{
	         elev.next_floor = downwards_destination;
		 elev.state = DOWN;
	     }
	 }else if(!no_one_waiting()){
	     //TODO make this find the closest people waiting on a floor and pick them up
         upwards = 0;
         downwards = 0;
         upwards_destination = MAX_FLOOR-1;
         downwards_destination = 0;
	 for(i = 0; i < MAX_FLOOR; i++){
	    for(j = 0; j < num_people[i]; j++){
	        if(elev.floor-i>0){
			downwards++;
			if(i>downwards_destination){
			   downwards_destination = i;
			}
		}else if(elev.floor-i<0){
			upwards++;
			if(i<upwards_destination){
			   upwards_destination = i;
			}

		}
	    }
	 }
	 //only set the destination if there was someone found in that direction
         if(upwards != 0 && downwards != 0){
            if(elev.floor-downwards >= upwards-elev.floor){
	       elev.next_floor = upwards_destination;
	       elev.state = UP;
	    }
	    else{
	       elev.next_floor = downwards_destination;
	       elev.state = DOWN;
	    }
	    
	 }else if(upwards == 0 && downwards == 0){
	       elev.state = IDLE;
	 }else if(upwards != 0){
	    elev.next_floor = upwards_destination;
	    elev.state = UP;
	 }else{
	    elev.next_floor = downwards_destination;
	    elev.state = DOWN;
	 }
         }else{
	    elev.state = IDLE;
	 }
     }
}


int elevator_proc_open(struct inode *sp_inode, struct file *sp_file){
     char* tmp_mess;
     //printk(KERN_INFO "Trying to open proc/timed\n");
     read_p = 1;
     message = kmalloc(sizeof(char)*2000, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
     tmp_mess = kmalloc(sizeof(char)*200, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
     if (message == NULL){
         printk(KERN_WARNING "elevator couldn't allocate any memory for messages\n");
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
     sprintf(tmp_mess,"Current floor: %d\n",elev.floor+1);
     message = strcat(message,tmp_mess);
     sprintf(tmp_mess,"Next floor: %d\n",elev.next_floor+1);
     message = strcat(message,tmp_mess);
     //sprintf(tmp_mess,"People on elevator: %d\n",elev.num_people);
     //message = strcat(message,tmp_mess);
     sprintf(tmp_mess,"Passenger weight: %d",elev.weight_units/2);
     message = strcat(message,tmp_mess);
     if(elev.weight_units%2==1){
        sprintf(tmp_mess,".5\n");
     }else{
        sprintf(tmp_mess,".0\n");
     }
     message = strcat(message,tmp_mess);
     sprintf(tmp_mess,"Passenger units: %d\n",elev.pass_units);
     message = strcat(message,tmp_mess);
     sprintf(tmp_mess,"Floor\tLoad of Waiting\t\tServiced\n");
     message = strcat(message,tmp_mess);
     for(i = 0; i < MAX_FLOOR; i++){
         sprintf(tmp_mess,"Floor[%d]:\t %d\t\t%d\n",i+1,num_people[i],number_serviced[i]);
         message = strcat(message,tmp_mess);
     
     }
     return 0;

}

ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
        int len = strlen(message);

	read_p = !read_p;
	if(read_p){
		return 0;
	}
        //printk(KERN_INFO "proc called read\n");
        copy_to_user(buf, message, len);
        return len;
}

int elevator_proc_release(struct inode *sp_inode, struct file *sp_file) {
        //printk(KERN_NOTICE "proc called release\n");
        kfree(message);
        return 0;
}

int move_that_elevator(void *data){
	while(!kthread_should_stop() || stopping){
	        ssleep(2);
		if(mutex_lock_interruptible(&thread1.my_mutex) == 0){
			//printk(KERN_ALERT "thread works fool ya fool\n");
		}
		//if the elevator has been requested to stop check that it isnt done yet
		if(stopping){
			if(elev.num_people == 0){
			          stopping = false;
				  elev.state = OFFLINE;
		                  mutex_unlock(&thread1.my_mutex);
				  return 1;
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
	////allocating pointer of pointers for floors
	//floors = (struct person**)vmalloc(MAX_FLOOR*sizeof(struct person*));
	//number_serviced = (int*)vmalloc(MAX_FLOOR*sizeof(int));
	//if(floors == NULL){
	//	printk(KERN_ALERT "memory allocation error\n");
	//   return -ENOMEM;
	//}
	////initializing the array holding how many people are waiting to 0 and floors[0-9] to null
	//num_people = (int*)vmalloc(MAX_FLOOR*sizeof(int));
	//for(i = 0; i < MAX_FLOOR; i++){
	//	floors[i] = NULL;
	//	number_serviced[i] = 0;
	//	num_people[i] = 0;
	//}
	////initializing the elev.people array to be able to hold 8 people since thats the max with the current weight and size limit
	//elev.people = (struct person*)vmalloc(8*sizeof(struct person));
	//if(elev.people == NULL){
	//	printk(KERN_ALERT "memory allocation error\n");
	//   return -ENOMEM;
	//}
	//if(floors == NULL || num_people == NULL || elev.people ==NULL){
	//	printk(KERN_ALERT "memory allocation error\n");
	//}
	////setting elevator starting states
	elev.state = IDLE;
	//elev.floor = 1;
	//elev.next_floor = 1;
	//elev.pass_units = 0;
	//elev.weight_units = 0;
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
	//to get the elevator moving
	if(elev.num_people ==0 && no_one_waiting()){
	   elev.next_floor = b;
	}
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
		i = num_people[b];
		if(a == ADULT){
			tmp[i].weight = 2;
			tmp[i].passenger = 1;
			tmp[i].destination_floor = c;
		//	tmp[i].title = a;
		}else if(a == CHILD){
			//we've changed everyones weight to double
			tmp[i].weight = 1;
			tmp[i].passenger = 1;
			tmp[i].destination_floor = c;
		//	tmp[i].title = a;
		}else if(a == ROOM_SERVICE){
			tmp[i].weight = 4;
			tmp[i].passenger = 1;
			tmp[i].destination_floor = c;
		//	tmp[i].title = a;
		}else if(a == BELL_HOP){
			tmp[i].weight = 4;
			tmp[i].passenger = 2;
			tmp[i].destination_floor = c;
		//	tmp[i].title = a;
		}
		else{
		    printk("error who is this\n");
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
	int ret;
        stopping = true;
	active = false;
        ret = kthread_stop(thread1.kthread);
        if(ret != -EINTR){
            printk("Thread stopped correctly\n");
        }
	return 0;
}

static int __init elevator_init(void){
     //started elevator states
     active = false;
     stopping = false;
     floors = (struct person**)vmalloc(MAX_FLOOR*sizeof(struct person*));
        number_serviced = (int*)vmalloc(MAX_FLOOR*sizeof(int));
        if(floors == NULL){
                printk(KERN_ALERT "memory allocation error\n");
           return -ENOMEM;
        }
        //initializing the array holding how many people are waiting to 0 and floors[0-9] to null
        num_people = (int*)vmalloc(MAX_FLOOR*sizeof(int));
        for(i = 0; i < MAX_FLOOR; i++){
                floors[i] = NULL;
                number_serviced[i] = 0;
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
        elev.floor = 0;
        elev.next_floor = 0;
        elev.pass_units = 0;
        elev.weight_units = 0;

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
     mutex_destroy(&thread1.my_mutex);
     STUB_start_elevator = NULL;
     STUB_stop_elevator = NULL;
     STUB_issue_request = NULL;
     //freeing memory
     for(i = 0; i < 10; i++){
     	vfree(floors[i]);
	if(floors[i]==NULL){
	   //printk("YOOOO floor %d is good\n",i);
	}
     }
     vfree(floors);
}



module_init(elevator_init);
module_exit(elevator_exit);
