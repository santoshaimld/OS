make
sudo rmmod simpleIpc_spsc
sudo insmod simpleIpc_spsc.ko
sudo chmod 666 /dev/ipc_dev
