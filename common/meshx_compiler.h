/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */
#ifndef _MESHX_COMPILER_H_
#define _MESHX_COMPILER_H_


#if defined ( __CC_ARM )
#define __ASM __asm /*!< asm keyword for armcc */
#define __INLINE __inline /*!< inline keyword for armcc */
#elif defined ( __ICCARM__ )
#define __ASM __asm /*!< asm keyword for iarcc */
#define __INLINE inline /*!< inline keyword for iarcc. Only
                                avaiable in High optimization mode! */
#define __nop __no_operation /*!< no operation intrinsic in iarcc */
#elif defined ( __GNUC__ )
#define __ASM asm /*!< asm keyword for gcc */
#define __INLINE inline /*!< inline keyword for gcc */
#endif

#define __WEAK __attribute((weak)) /*!< weak keywork */
#define __PACKED __attribute__((__packed__))  /*!< packed keyword */

#endif
