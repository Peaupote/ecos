#!/bin/sh

MNT=/mnt
IMG=home.img
BASE=home

if [ -e $IMG ];
then
    echo "Partition $IMG already exists";
    dumpe2fs $IMG
    exit 0;
else
    echo "Creating partition $IMG";
    dd if=/dev/zero of=$IMG bs=64K count=1;
    mke2fs $IMG
fi

if [ ! -e $MNT ];
then mkdir $MNT
fi

mount $IMG $MNT &&
    mkdir $MNT/mpouzet &&
    cp -r $BASE/* $MNT/
    umount $MNT &&
    echo "Partition created !"
