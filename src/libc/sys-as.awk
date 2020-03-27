BEGIN {
    print("// auto-generated file containing syscall interruptions")
    print("#define ASM_FILE")
    print("#include<kernel/sys.h>")
    print("#include<kernel/int.h>")
    print(".section .text")
}
$1 == "#define" && $2 ~ /SYS_R1/ && $3 ~ /[0-9]+/ {
    printf(".global %s\n", substr(tolower($2), 5));
    printf("%s:\n\t", substr(tolower($2), 5));
    printf("movq $%s, %rax\n\t", $2);
    printf("int  $SYSCALL_R1_VEC\n");
	printf("retq\n\n")
	next
}
$1 == "#define" && $2 ~ /SYS_/ && $3 ~ /[0-9]+/ {
	printf(".global %s\n", substr(tolower($2), 5));
	printf("%s:\n\t", substr(tolower($2), 5));
	printf("movq $%s, %rax\n\t", $2);
	printf("int  $SYSCALL_VEC\n");
	printf("retq\n\n")
}
