KDIR  		:= /lib/modules/$(shell uname -r)/build
PWD			:= $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=kernel/lib/ modules_install 

%.ko:
	
	make -C $(KDIR) M=$(PWD) $@
	-rm *.mod.[co]
	
clean:
	make -C $(KDIR) M=$(PWD) $@

