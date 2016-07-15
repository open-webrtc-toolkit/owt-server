/**
 * @file re_md5.h  Interface to MD5 functions
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** MD5 values */
enum {
	MD5_SIZE     = 16,             /**< Number of bytes in MD5 hash   */
	MD5_STR_SIZE = 2*MD5_SIZE + 1  /**< Number of bytes in MD5 string */
};

void md5(const uint8_t *d, size_t n, uint8_t *md);
int  md5_printf(uint8_t *md, const char *fmt, ...);
