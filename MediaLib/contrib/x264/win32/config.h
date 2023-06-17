#define HAVE_X86_INLINE_ASM 0
#define HAVE_MMX 1

#if _WIN64
#define ARCH_X86 0
#define ARCH_X86_64 1
#define STACK_ALIGNMENT 16
#else
#define ARCH_X86 1
#define ARCH_X86_64 0
#define STACK_ALIGNMENT 4
#endif

#define SYS_WINDOWS 1
#define HAVE_WIN32THREAD 1
#define HAVE_THREAD 1
#define HAVE_LOG2F 0
#define HAVE_AVS 0
#define USE_AVXSYNTH 0
#define HAVE_VECTOREXT 1
#define fseek _fseeki64
#define ftell _ftelli64
#define HAVE_GPL 1
#define HAVE_INTERLACED 1
#define HAVE_OPENCL 0
#define HAVE_MALLOC_H 0
#define HAVE_ALTIVEC 0
#define HAVE_ALTIVEC_H 0
#define HAVE_ARMV6 0
#define HAVE_ARMV6T2 0
#define HAVE_NEON 0
#define HAVE_BEOSTHREAD 0
#define HAVE_POSIXTHREAD 0
#define HAVE_SWSCALE 0
#define HAVE_LAVF 0
#define HAVE_FFMS 0
#define HAVE_GPAC 0
#define HAVE_CPU_COUNT 0
#define HAVE_THP 0
#define HAVE_LSMASH 0
#define HAVE_AS_FUNC 0
#define HAVE_INTEL_DISPATCHER 0
#define HAVE_MSA 0
#define HAVE_MMAP 0
#define HAVE_WINRT 0
#define HAVE_VSX 0

#define	isfinite     _finite
