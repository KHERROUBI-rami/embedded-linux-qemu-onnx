#!/bin/sh
cd ~/arm-lab
sudo mount -o loop rootfs.img /mnt
sudo cp -a rootfs/. /mnt/
sudo chown -R 0:0 /mnt
sudo umount /mnt
