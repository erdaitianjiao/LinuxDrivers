KERNELDIR := /home/tianjiao/workspace/linux/IMX6ULL/linux/origin_linux/tianjiaoorlinux/
CURRENT_PATH := $(shell pwd)

obj-m := led.o

build: kernel_modules

kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean