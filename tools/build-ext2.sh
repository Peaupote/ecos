#!/bin/sh

# Usage: build-ext2 input-dir output.img
# requiert `e2tools`

BASE="$1"
IMG="$2"
IMG_ABS="`pwd`/$IMG"

rm -f "$IMG"

echo "Creating partition $IMG";
dd if=/dev/zero of="$IMG" bs=64K count=1;
mke2fs "$IMG"

if ! cd "$BASE"
then
	rm -f "$IMG"
	exit 1
fi

find ./ -type d -exec e2mkdir "$IMG_ABS":{} \; \
	&& find ./ -type f -exec e2cp {} "$IMG_ABS":{} \; \
    && echo "Partition created !"

if [ $? -ne 0 ]
then
	rm -f "$IMG_ABS"
	exit 1
else
	exit 0
fi
