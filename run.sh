#!/bin/sh
cd ~/arm-lab
qemu-system-aarch64 -M virt -cpu cortex-a57 -m 1024 \
  -kernel linux-6.6/arch/arm64/boot/Image \
  -drive file=rootfs.img,format=raw,if=virtio \
  -append "root=/dev/vda rw console=ttyAMA0 init=/sbin/init" \
  -nographic \
  -netdev user,id=n1,hostfwd=tcp::2222-:22,hostfwd=tcp::8080-:80 \
  -device virtio-net-pci,netdev=n1
