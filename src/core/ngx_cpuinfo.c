
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


#if (( __i386__ || __amd64__ ) && ( __GNUC__ || __INTEL_COMPILER ))


static ngx_inline void ngx_cpuid(uint32_t i, uint32_t *buf);


#if ( __i386__ )

static ngx_inline void
ngx_cpuid(uint32_t i, uint32_t *buf)
{

    /*
     * we could not use %ebx as output parameter if gcc builds PIC,
     * and we could not save %ebx on stack, because %esp is used,
     * when the -fomit-frame-pointer optimization is specified.
     */

    __asm__ (

    "    mov    %%ebx, %%esi;  "

    "    cpuid;                "
    "    mov    %%eax, (%1);   "
    "    mov    %%ebx, 4(%1);  "
    "    mov    %%edx, 8(%1);  "
    "    mov    %%ecx, 12(%1); "

    "    mov    %%esi, %%ebx;  "

    : : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory" );
}


#else /* __amd64__ */


static ngx_inline void
ngx_cpuid(uint32_t i, uint32_t *buf)
{
    uint32_t  eax, ebx, ecx, edx;

    __asm__ (

        "cpuid"

    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i) );

    buf[0] = eax;
    buf[1] = ebx;
    buf[2] = edx;
    buf[3] = ecx;
}


#endif


/* auto detect the L2 cache line size of modern and widespread CPUs by using
 * CPUID.
 * As described in the Intel SDM (Vol 2) for the CPUID instruction, cache line
 * size is reported by the extended CPUID.80000006H function, returned in
 * ECX[00-07].
 * Cache line size (for CLFLUSH) is also reported in CPUID.01H, returned in
 * EBX[08-15], to be multiplied by 8.
 */
void
ngx_cpuinfo(void)
{
    uint32_t   regs[4], maxfn;

    /* Determine the maximum extended CPUID function level */
    ngx_cpuid(0x80000000, regs);
    maxfn = regs[0];

    if (maxfn >= 0x80000006) {
        /* Cache line size by CPUID.80000006H, in ECX[00-07] */
        ngx_cpuid(0x80000006, regs);
        ngx_cacheline_size = regs[3] & 0xff;
        return;
    }

    /* Insufficient extended functions, so fall back to basic functions */
    ngx_cpuid(0, regs);
    maxfn = regs[0];

    if (maxfn >= 1) {
        ngx_cpuid(1, regs);
        ngx_cacheline_size = ((regs[1] & 0xff00) >> 8) << 3;
    } else {
        /* This should never execute. Misconfigured hypervisor/emulator? */
        ngx_cacheline_size = 64;
    }
}

#else


void
ngx_cpuinfo(void)
{
}


#endif
