#!/usr/bin/awk -f

/my_/{
	match($0, /my_([[:alnum:]]+) *\(/, a);
	printf("--redefine-sym %s=my_%s ",a[1],a[1]);
}
