#!/bin/sh

# requiert `e2tools`

IMG=home.img
BASE=home

rm -f $IMG

echo "Creating partition $IMG";
dd if=/dev/zero of=$IMG bs=64K count=1;
mke2fs $IMG

if ! cd $BASE
then
	rm -f $IMG
fi

find ./ -type d -exec e2mkdir ../$IMG:{} \; \
	&& find ./ -type f -exec e2cp {} ../$IMG:{} \; \
    && echo "Partition created !"

if [ $? -ne 0 ]
then
	rm -f ../$IMG
fi
