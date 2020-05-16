$(ROOT)/vars_local.Makefile:
	touch $(ROOT)/vars_local.Makefile

export NAME_LC?=libc
export NAME_LK?=libk
export TSFX?=

include $(ROOT)/vars_local.Makefile

# Semble empêcher gcc d'utiliser des instructions utilisant
# les flottants comme optimisation
FIX_FLOAT_OPT?=0

ifeq ($(FIX_FLOAT_OPT), 1)
# options may not be valid for your GCC
# you can find which one your GCC know with.
#   gcc -Q --help=optimizers -O2
OPT?=\
  -faggressive-loop-optimizations	\
  -falign-functions					\
  -falign-jumps						\
  -falign-labels					\
  -falign-loops						\
  -fasynchronous-unwind-tables		\
  -fauto-inc-dec					\
  -fbranch-count-reg				\
  -fcaller-saves					\
  -fcode-hoisting					\
  -fcombine-stack-adjustments		\
  -fcompare-elim					\
  -fcprop-registers					\
  -fcrossjumping					\
  -fcse-follow-jumps				\
  -fdce								\
  -fdefer-pop						\
  -fdelete-null-pointer-checks		\
  -fdevirtualize					\
  -fdevirtualize-speculatively		\
  -fdse								\
  -fearly-inlining					\
  -fexpensive-optimizations			\
  -fforward-propagate				\
  -ffp-int-builtin-inexact			\
  -ffunction-cse					\
  -fgcse							\
  -fgcse-lm							\
  -fguess-branch-probability		\
  -fhoist-adjacent-loads			\
  -fif-conversion					\
  -fif-conversion2					\
  -findirect-inlining				\
  -finline							\
  -finline-atomics					\
  -finline-functions-called-once	\
  -finline-small-functions			\
  -fipa-bit-cp						\
  -fipa-cp							\
  -fipa-icf							\
  -fipa-icf-functions				\
  -fipa-icf-variables				\
  -fipa-profile						\
  -fipa-pure-const					\
  -fipa-ra							\
  -fipa-reference					\
  -fipa-reference-addressable		\
  -fipa-sra							\
  -fipa-stack-alignment				\
  -fipa-vrp							\
  -fira-hoist-pressure				\
  -fira-share-save-slots			\
  -fira-share-spill-slots			\
  -fisolate-erroneous-paths-dereference		\
  -fivopts							\
  -fjump-tables						\
  -flifetime-dse					\
  -flra-remat						\
  -fmath-errno						\
  -fmove-loop-invariants			\
  -fomit-frame-pointer				\
  -foptimize-sibling-calls			\
  -foptimize-strlen					\
  -fpartial-inlining				\
  -fpeephole						\
  -fpeephole2						\
  -fplt								\
  -fprintf-return-value				\
  -freg-struct-return				\
  -frename-registers				\
  -freorder-blocks					\
  -freorder-blocks-and-partition	\
  -freorder-functions				\
  -frerun-cse-after-loop			\
  -fsched-critical-path-heuristic	\
  -fsched-dep-count-heuristic		\
  -fsched-group-heuristic			\
  -fsched-interblock				\
  -fsched-last-insn-heuristic		\
  -fsched-rank-heuristic			\
  -fsched-spec						\
  -fsched-spec-insn-heuristic		\
  -fsched-stalled-insns-dep			\
  -fschedule-fusion					\
  -fschedule-insns2					\
  -fshort-enums						\
  -fshrink-wrap						\
  -fshrink-wrap-separate			\
  -fsigned-zeros					\
  -fsplit-ivs-in-unroller			\
  -fsplit-wide-types				\
  -fssa-backprop					\
  -fssa-phiopt						\
  -fstdarg-opt						\
  -fstore-merging					\
  -fstrict-aliasing					\
  -fstrict-volatile-bitfields		\
  -fthread-jumps					\
  -ftrapping-math					\
  -ftree-bit-ccp					\
  -ftree-builtin-call-dce			\
  -ftree-ccp						\
  -ftree-ch							\
  -ftree-coalesce-vars				\
  -ftree-copy-prop					\
  -ftree-cselim						\
  -ftree-dce						\
  -ftree-dominator-opts				\
  -ftree-dse						\
  -ftree-forwprop					\
  -ftree-fre						\
  -ftree-loop-if-convert			\
  -ftree-loop-im					\
  -ftree-loop-ivcanon				\
  -ftree-loop-optimize				\
  -ftree-phiprop					\
  -ftree-pre						\
  -ftree-pta						\
  -ftree-reassoc					\
  -ftree-scev-cprop					\
  -ftree-sink						\
  -ftree-slsr						\
  -ftree-sra						\
  -ftree-switch-conversion			\
  -ftree-tail-merge					\
  -ftree-ter						\
  -ftree-vrp						\
  -funwind-tables					\
  -fweb
else
OPT?=-O2
endif

#Communs
FLAGS_C?=$(OPT) -g -Wall -Wextra -std=gnu99 -nostdlib -mno-red-zone -D__is_debug
#Kernel
FLAGS_K?=$(FLAGS_C) -mcmodel=large -ffreestanding -D__is_kernel
#Boot
FLAGS_B?=$(FLAGS_C) -ffreestanding
#Lib
FLAGS_L?=$(FLAGS_C) -ffreestanding
#LibC
export FLAGS_LC?=$(FLAGS_L)
#LibK
export FLAGS_LK?=$(FLAGS_L) -mcmodel=large -D__is_kernel
#Userspace
export FLAGS_UR?=$(FLAGS_C) -fno-builtin -ffreestanding

export LINK_LC=-Xlinker "--just-symbols=$(ROOT)/src/libc/libc.sym"

VARS_NAME=NAME_LC NAME_LK TSFX FLAGS_C FLAGS_K FLAGS_B FLAGS_L FLAGS_LC \
		  FLAGS_LK FLAGS_UR INCLUDE_LIBC
