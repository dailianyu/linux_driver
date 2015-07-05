ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /lib/modules/$(shell uname -r)/build
KERNELDIR ?= /home/linux/tq/kernel/linux-2.6.32.10-tq2440

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules modules_install clean

else
    obj-m := ioctl_drv.o
endif

