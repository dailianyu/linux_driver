ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /opt/linux-2.6.32.10_ljk/

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules modules_install clean

else
    obj-m := final.o
    final-objs :=init.o cleanup.o
endif

