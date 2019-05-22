/**
 * This file is part of the meshx library.
 *
 * Copyright 2019, Huang Yang <elious.huang@gmail.com>. All rights reserved.
 *
 * See the LICENSE file for the terms of usage and distribution.
 */
#ifndef _MESHX_ENDIANNESS_H_
#define _MESHX_ENDIANNESS_H_

#define MESHX_HOST_TO_LE16(data) (data)
#define MESHX_HOST_TO_LE32(data) (data)
#define MESHX_HOST_TO_BE16(data) ((((data) & 0xff00) >> 8) | \
                                  (((data) & 0x00ff) << 8))
#define MESHX_HOST_TO_BE32(data) ((((data) & 0xff000000) >> 24) | \
                                  (((data) & 0x00ff0000) >> 8) | \
                                  (((data) & 0x0000ff00) << 8) | \
                                  (((data) & 0x000000ff) << 24))

#define MESHX_LE16_TO_HOST(data) (data)
#define MESHX_LE32_TO_HOST(data) (data)
#define MESHX_BE16_TO_HOST(data) ((((data) & 0xff00) >> 8) | \
                                  (((data) & 0x00ff) << 8))
#define MESHX_BE32_TO_HOST(data) ((((data) & 0xff000000) >> 24) | \
                                  (((data) & 0x00ff0000) >> 8) | \
                                  (((data) & 0x0000ff00) << 8) | \
                                  (((data) & 0x000000ff) << 24))



#endif /* _MESHX_ENDIANNESS_H_ */
