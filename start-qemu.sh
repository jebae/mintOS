#!/bin/bash

if [[ "$1" == "hdd" ]]; then
	qemu-system-x86_64 -m 64 -L .\
		-drive file=./Disk.img,if=floppy,format=raw\
		-hda ./HDD.img
else
	qemu-system-x86_64 -m 64 -L .\
		-drive file=./Disk.img,if=floppy,format=raw
fi
