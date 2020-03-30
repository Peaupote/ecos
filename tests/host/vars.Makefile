export TRG=host
export CC=gcc
export AS=as
export AR=ar
export NAME_LC=tlibc
export NAME_LK=tlibk

FLAGS_C=-g -O1 -Wall -Wextra -std=gnu99 -mno-red-zone -fno-builtin -D__is_test
export FLAGS_LC=$(FLAGS_C)
export FLAGS_LK=$(FLAGS_C) -D__is_kernel
export FLAGS_T=$(FLAGS_C)
