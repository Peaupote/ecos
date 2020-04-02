export NAME_LC?=libc
export NAME_LK?=libk
export TSFX?=

#Communs
FLAGS_C?=-O1 -Wall -Wextra -std=gnu99 -nostdlib -mno-red-zone
#Kernel
FLAGS_K?=$(FLAGS_C) -mcmodel=large -ffreestanding -D__is_kernel
#Boot
FLAGS_B?=$(FLAGS_C) -ffreestanding
#Lib
FLAGS_L?=$(FLAGS_C) -mcmodel=large -ffreestanding
#LibC
export FLAGS_LC?=$(FLAGS_L)
#LibK
export FLAGS_LK?=$(FLAGS_L) -D__is_kernel
#Userspace
export FLAGS_UR?=$(FLAGS_C) -fno-builtin -ffreestanding

VARS_NAME=NAME_LC NAME_LK TSFX FLAGS_C FLAGS_K FLAGS_B FLAGS_L FLAGS_LC \
		  FLAGS_LK FLAGS_UR INCLUDE_LIBC
