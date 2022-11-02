MODULE_NAME = Args

obj-m = $(MODULE_NAME).o

PWD := $(CURDIR)
TMP = $(MODULE_NAME).mod $(MODULE_NAME).mod.c $(MODULE_NAME).mod.o $(MODULE_NAME).o
TMP += .$(MODULE_NAME).ko.cmd .$(MODULE_NAME).mod.cmd .$(MODULE_NAME).mod.o.cmd
TMP += .$(MODULE_NAME).o.cmd .Module.symvers.cmd .modules.order.cmd
TMP += Module.symvers modules.order

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	mkdir -pv tmp
	mv -v $(TMP) tmp/

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rfv tmp/

.PHONY: all clean
