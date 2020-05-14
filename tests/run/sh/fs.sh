#!/bin/sh

echo -e "\e[1;37mls /\e[0m"
ls /

echo -e "\n\e[1;37mls /bin\e[0m"
ls /bin

echo -e "\n\e[1;37m\$PATH:\e[0m $PATH"
echo -e "\e[1;37mls /proc\e[0m"
ls /proc

echo -e "\n\e[1;37mls /proc/5\e[0m"
ls /proc/5

echo -ne "\n\e[1;37m/proc/5/stat:\e[0m "
cat /proc/5/stat
