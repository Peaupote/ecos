export TRG=host
export TSFX=.test
export CC=gcc
export AS=as
export AR=ar
export NAME_LC=tlibc
export NAME_LK=tlibk

FLAGS_C=-g -O0 -Wall -Wextra -std=gnu99 -D__is_test
export FLAGS_LC=$(FLAGS_C) -fno-builtin -nostdlib
export FLAGS_LK=$(FLAGS_LC) -D__is_kernel
export FLAGS_T=$(FLAGS_C)
export FLAGS_TU=$(FLAGS_C) -D__is_test_unit

export INCLUDE_LIBC=
