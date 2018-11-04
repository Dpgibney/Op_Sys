#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
/* System call stub */
int (*STUB_start_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
/* System call wrapper */
asmlinkage int sys_start_elevator(void){
if (STUB_start_elevator != NULL)
	return STUB_start_elevator();
else
	return -1;
}
int (*STUB_issue_request)(int a, int b, int c) = NULL;
EXPORT_SYMBOL(STUB_issue_request);
/* System call wrapper */
asmlinkage int sys_issue_request(int a, int b, int c){
if (STUB_issue_request != NULL)
	return STUB_issue_request(a,b,c);
else
	return -1;
}
int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_stop_elevator);
/* System call wrapper */
asmlinkage int sys_stop_elevator(void){
if (STUB_stop_elevator != NULL)
	return STUB_stop_elevator();
else
	return -1;
}
