#!/bin/bash

#Usage: test_section.sh DIR "NAME@FILE.tst"

awkscr=`dirname "$0"`
awkscr="$awkscr/section.awk"
parts=`sed -n 's/^\([[:alpha:]0-9._]*\)@\([[:alpha:]0-9.\/_]*\).tst$/\1 \2/p' <<< "$2"`
if [[ -n "$parts" ]]
then
	sname=`cut -d ' ' -f 1 <<< "$parts"`
	sname="TEST_SECTION_$sname"
	fname=`cut -d ' ' -f 2 <<< "$parts" | sed -e 's/__/\//g'`
	fname="$1/$fname"
	if [[ -f "$fname" ]]
	then
		(echo "//section $sname @ $fname"; gawk -f "$awkscr" -v sname="$sname" "$fname") > "$2"
		exit 0
	else
		echo "$fname n'existe pas" 1>&2
		exit 1
	fi
fi
echo "erreur de parsing du nom de fichier .tst" 1>&2
exit 2
