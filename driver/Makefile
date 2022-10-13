obj-m += multiflow_driver.o
mymodule-objs := read_write_functions.o lock.o multiflow_driver.o structs.o values.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean