/**
 * @file re_crc32.h  Interface to CRC-32 functions
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifdef USE_ZLIB
#include <zlib.h>
#else
uint32_t crc32(uint32_t crc, const void *buf, uint32_t size);
#endif
