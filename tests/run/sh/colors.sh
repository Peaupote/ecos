#!/bin/sh

echo -ne "0\n1\n2\n3\n4\n5\n6\n7\n" > ~/codes.txt
list_c="0 1 2 3 4 5 6 7"

echo -n " "
for i in 1 2; do
	for fg in ${list_c}; do echo -n $fg; done
done
echo

while read bg; do
	echo -n ${bg}
	while read fg; do
		echo -ne "\e[3${fg};4${bg}m#\e[0m"
	done < ~/codes.txt
	while read fg; do
		echo -ne "\e[4${bg};1;3${fg}m#\e[0m"
	done < ~/codes.txt
	echo
done < ~/codes.txt
for bg in ${list_c}; do
	echo -n ${bg}
	for fg in ${list_c}; do
		echo -ne "\e[3${fg};1;4${bg}m#\e[0m"
	done
	for fg in ${list_c}; do
		echo -ne "\e[1;4${bg};3${fg}m#\e[0m"
	done
	echo
done
