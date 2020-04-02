#!/bin/sh

# Usage: build-ext2 input-dir output.img
# requiert `e2tools`

BASE="$1"
IMG="$2"
IMG_ABS="`pwd`/$IMG"

rm -f "$IMG"

echo "Creating partition $IMG";
dd if=/dev/zero of="$IMG" bs=64K count=8;
mke2fs "$IMG"

if ! cd "$BASE"
then
	rm -f "$IMG"
	exit 1
fi

find ./ -type d | tail -n +2 | while read -r dir ; do
	if ! e2mkdir "$IMG_ABS:$dir" ; then
		exit 1
	fi
done

if [ $? -ne 0 ]
then
	rm -f "$IMG_ABS"
	exit 1
fi

find ./ -type f | while read -r file ; do
	if ! e2cp "$file" "$IMG_ABS:$file" ; then
		exit 1
	fi
done

if [ $? -ne 0 ]
then
	rm -f "$IMG_ABS"
	exit 1
else
	echo "Partition created !"
	exit 0
fi
