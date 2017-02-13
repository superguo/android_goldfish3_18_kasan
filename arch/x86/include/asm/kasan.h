#ifndef _ASM_X86_KASAN_H
#define _ASM_X86_KASAN_H

#include <linux/const.h>
#define KASAN_SHADOW_OFFSET _AC(CONFIG_KASAN_SHADOW_OFFSET, UL)

#define KASAN_SHADOW_START      (0xffffec0000000000ULL)
/* 47 bits for kernel address -> (47 - 3) bits for shadow */
#define KASAN_SHADOW_END        (KASAN_SHADOW_START + (1ULL << (47 - 3)))

#ifndef __ASSEMBLY__

#ifdef CONFIG_KASAN
void __init kasan_early_init(void);
void __init kasan_init(void);
#else
static inline void kasan_early_init(void) { }
static inline void kasan_init(void) { }
#endif

#endif

#endif
