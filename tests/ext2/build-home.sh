#!/bin/sh

# requiert `e2tools`

IMG=home.img
BASE=home

if [ -e $IMG ];
then
    echo "Partition $IMG already exists";
    dumpe2fs $IMG
    exit 0;
fi
   
echo "Creating partition $IMG";
dd if=/dev/zero of=$IMG bs=64K count=1;
mke2fs $IMG

if ! cd $BASE
then
	rm -f $IMG
fi

e2mkdir ../$IMG:mpouzet \
	&& find ./ -type f -exec e2cp {} ../$IMG:{} \; \
    && echo "Partition created !"

if [ $? -ne 0 ]
then
	rm -f ../$IMG
fi
