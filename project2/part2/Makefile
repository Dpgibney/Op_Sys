ifneq ($(KERNELRELEASE),)
	obj-m := my_xtime.o
else
	KERNELDIR ?= /lib/modules/`uname -r`/build/
	PWD := `pwd`
default:
	$(MAKE) CONFIG_STACK_VALIDATION= -C $(KERNELDIR) M=$(PWD) modules
endif
clean:
	rm -f *.ko *.o Module* *mod*
