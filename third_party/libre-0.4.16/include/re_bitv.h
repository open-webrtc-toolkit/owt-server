/**
 * @file re_bitv.h  Interface to Bit Vector functions
 *
 * Copyright (C) 2010 Creytiv.com
 */


typedef unsigned long bitv_t;

enum {
	BITS_SZ   = (8*sizeof(bitv_t)),
	BITS_MASK = (BITS_SZ-1)
};

#define BITV_NELEM(nbits) (((nbits) + (BITS_SZ) - 1) / (BITS_SZ))
#define BITV_DECL(name, nbits) bitv_t (name)[BITV_NELEM(nbits)]


static inline uint32_t index2offset(uint32_t i)
{
	return i/BITS_SZ;
}

static inline bitv_t index2bit(uint32_t i)
{
	return (bitv_t)1<<(i & BITS_MASK);
}


/*
 * Public API
 */


static inline void bitv_init(bitv_t *bv, uint32_t nbits, bool val)
{
	memset(bv, val?0xff:0x00, BITV_NELEM(nbits)*sizeof(bitv_t));
}

static inline void bitv_set(bitv_t *bv, uint32_t i)
{
	bv[index2offset(i)] |= index2bit(i);
}

static inline void bitv_clr(bitv_t *bv, uint32_t i)
{
	bv[index2offset(i)] &= ~index2bit(i);
}

static inline void bitv_assign(bitv_t *bv, uint32_t i, bool val)
{
	if (val)
		bitv_set(bv, i);
	else
		bitv_clr(bv, i);
}

static inline bool bitv_val(const bitv_t *bv, uint32_t i)
{
	return 0 != (bv[index2offset(i)] & index2bit(i));
}

static inline void bitv_toggle(bitv_t *bv, uint32_t i)
{
	bv[index2offset(i)] ^= index2bit(i);
}

static inline void bitv_assign_range(bitv_t *bv, uint32_t i, uint32_t n,
				     bool val)
{
	while (n--)
		bitv_assign(bv, i+n, val);
}
