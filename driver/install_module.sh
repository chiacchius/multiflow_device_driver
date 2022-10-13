sudo rm /dev/multiflow_device*
sudo rmmod multiflow_driver.ko
make clean
make all
sudo insmod multiflow_driver.ko
make clean
