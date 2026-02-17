make
sudo rmmod simpleIpc_mpmc_l
sudo insmod simpleIpc_mpmc_l.ko
sudo chmod 666 /dev/ipc_dev
