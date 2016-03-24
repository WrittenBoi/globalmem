KERNELVERSION = $(shell uname -r)

# kernel module
obj-m += globalmem.o

#EXTRA_CFLAGS = -g -O0

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KERNELVERSION)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(KERNELVERSION)/build M=$(CURDIR) clean
