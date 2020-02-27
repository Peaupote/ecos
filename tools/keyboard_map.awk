#!/usr/bin/awk -f
BEGIN{FS="\t"; num=0;}
/^0/{num++;printf(".byte 0, 0, 0, 0\n", $2)}
/^1\t/{num++;printf(".byte %s, 0, 0, 0\n", $2)}
/^2\t/{num++;printf(".byte %s, %s, 0, 0\n", $2, $3)}
/^3\t/{num++;printf(".byte %s, %s, %s, 0\n", $2, $3, $4)}
/^4\t/{num++;printf(".byte %s, %s, %s, %s\n", $2, $3, $4, $5)}
/^a\t/{num++;printf(".byte '%s', '%s', 0, 0\n", $2, toupper($2))}
END{if(num < 128) printf(".fill %d, 4, 0\n", 128 - num);}
