#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/time.h>
MODULE_LICENSE("Dual BSD/GPL");
#define TIME_SIZE 200
#define ENTRY_NAME "timed"
#define PERMS 0644

static struct file_operations fops;
struct timespec start_time, currenter_time;


static char *message;
static int read_p;


int my_xtime_proc_open(struct inode *sp_inode, struct file *sp_file){
     printk(KERN_INFO "Trying to open proc/timed\n");
     currenter_time = current_kernel_time();
     read_p = 1;
     message = kmalloc(sizeof(char)*TIME_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
     if (message == NULL){
         printk(KERN_WARNING "my_xtimed couldn't allocate any memory for messages\n");
	 return -ENOMEM;
     }
     if(currenter_time.tv_nsec-start_time.tv_nsec < 0){
     	sprintf(message,"current time1: %ld.%ld\nelapsed time: %ld.%ld\n",((currenter_time.tv_sec-start_time.tv_sec)-1),(1000000000+currenter_time.tv_nsec-start_time.tv_nsec),start_time.tv_sec,start_time.tv_nsec);
	}else{

     sprintf(message,"current time2: %ld.%ld\nelapsed time: %ld.%ld\n",((currenter_time.tv_sec-start_time.tv_sec)),((currenter_time.tv_nsec-start_time.tv_nsec)),start_time.tv_sec,start_time.tv_nsec);
	}
	 return 0;

}
ssize_t my_xtime_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	printk(KERN_INFO "proc called read\n");
	copy_to_user(buf, message, len);
	return len;
}
int my_xtime_proc_release(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_NOTICE "proc called release\n");
	kfree(message);
	return 0;
}

static int my_xtime_init(void){
    printk(KERN_ALERT "Start of my_xtime\n");
	start_time = current_kernel_time();
   	fops.open = my_xtime_proc_open;
	fops.read = my_xtime_proc_read;
	fops.release = my_xtime_proc_release;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "proc create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}

    return 0;
}
static void my_xtime_exit(void){
	remove_proc_entry(ENTRY_NAME, NULL);
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
    printk(KERN_ALERT "End of my_xtime\n");
}
module_init(my_xtime_init);
module_exit(my_xtime_exit);
