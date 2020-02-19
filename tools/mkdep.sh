#!/bin/bash

#Usage: mkdep.sh "gcc flags" src1.c src2.c ...
#pour inclure des flags spécifiques à un argument:
#	"flags1 src1.c"

gcc="$1"
shift 1
for arg in "$@"
do
	$gcc -MM -MG -M $arg
done
