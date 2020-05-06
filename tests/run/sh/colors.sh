#!/bin/sh

echo -e "0\n1\n2\n3\n4\n5\n6\n7\n" > ~/codes.txt

while read bg; do
	while read fg; do
		echo -ne "\e[3${fg};4${bg}m#\e[0m"
	done < ~/codes.txt
	while read fg; do
		echo -ne "\e[4${bg};1;3${fg}m#\e[0m"
	done < ~/codes.txt
	echo
done < ~/codes.txt
while read bg; do
	while read fg; do
		echo -ne "\e[3${fg};1;4${bg}m#\e[0m"
	done < ~/codes.txt
	while read fg; do
		echo -ne "\e[1;4${bg};3${fg}m#\e[0m"
	done < ~/codes.txt
	echo
done < ~/codes.txt
