#!/usr/bin/awk -f

/my_/{
	match($0, /my_([[:alnum:]_]+) *\(/, a);
	printf("--redefine-sym %s=my_%s ",a[1],a[1]);
}
