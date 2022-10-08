sudo rm /dev/multiflow_driver*
sudo rmmod multiflow_driver.ko
make clean
make all
sudo insmod multiflow_driver.ko
make clean
