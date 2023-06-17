%define ARCH_AARCH64 0
%define ARCH_ALPHA 0
%define ARCH_ARM 0
%define ARCH_AVR32 0
%define ARCH_AVR32_AP 0
%define ARCH_AVR32_UC 0
%define ARCH_BFIN 0
%define ARCH_IA64 0
%define ARCH_M68K 0
%define ARCH_MIPS 0
%define ARCH_MIPS64 0
%define ARCH_PARISC 0
%define ARCH_PPC 0
%define ARCH_PPC64 0
%define ARCH_S390 0
%define ARCH_SH4 0
%define ARCH_SPARC 0
%define ARCH_SPARC64 0
%define ARCH_TILEGX 0
%define ARCH_TILEPRO 0
%define ARCH_TOMI 0
%define ARCH_X86 1
;--------------------------------
%ifdef _MAC64;GSI-2018-2 ???
%define ARCH_X86_32 0
%define ARCH_X86_64 1
%else
%ifdef _WIN64
%define ARCH_X86_32 0
%define ARCH_X86_64 1
%else
%define ARCH_X86_32 1
%define ARCH_X86_64 0
%endif
%endif;_MAC64
;--------------------------------
%ifdef _MAC64;GSI-2018-2 ???
%define STACK_ALIGNMENT 16
%else
%ifdef _WIN64
%define STACK_ALIGNMENT 16
%else
%define STACK_ALIGNMENT 4
%endif
%endif;_MAC64
%define HIGH_BIT_DEPTH 0
%define BIT_DEPTH 8
