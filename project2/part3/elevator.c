#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/time.h>
MODULE_LICENSE("Dual BSD/GPL");
#define MAX_WEIGHT 10
#define MAX_PASS_UNITS 8
#define MAX_FLOOR 10
#define MIN_FLOOR 1
#define LOAD_WAIT 1
#define FLOOR_CHANGE_TIME 2

bool active;
struct elevator{
     int floor;
     enum {IDLE, OFFLINE, LOADING, UP, DOWN} state;
     struct person* people;
     int pass_units;
     int weight_units;
};
struct person{
     int weight;
     int passenger;
     enum {ADULT, CHILD, ROOM_SERVICE, BELL_HOP} title;
};

struct elevator elev;

int start_elevator(void){
     if(active==true){
        return 1;
     }else{
	elev.people = (struct person*)kmalloc(8*sizeof(struct person), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if(elev.people == NULL){
	   return -ENOMEM;
	}
	elev.state = IDLE;
	elev.floor = 1;
	elev.pass_units = 0;
	elev.weight_units = 0;
        return 0;
     }
}

static int elevator_init(void){
     active = false;
     return 0;
}
static void elevator_exit(void){

}

module_init(elevator_init);
module_exit(elevator_exit);

