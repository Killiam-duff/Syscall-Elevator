obj-m += src/my_timer.o

ccflags-y := -g -Wno-error

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd)  modules

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
