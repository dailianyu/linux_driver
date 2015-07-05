ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /your/target/source/directory/
KERNELDIR ?= /opt/linux-2.6.32.10

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules modules_install clean

else
    obj-m := mypipe.o
endif

