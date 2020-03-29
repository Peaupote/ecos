#!/bin/bash

kbin='./src/kernel/kernel.bin'
bbin='./src/kernel/kernel.bin'

echo -n "Check kernel.bin..."
end=`./tools/elf_info.out e "$kbin"`
lim="0x"`readelf -s "$kbin" | gawk '/kernel_uplim/{print $2}'`

if [[ $end -gt $lim ]]
then
	echo " $end > $lim fail"
	exit 1
fi
echo " $end <= $lim done"
