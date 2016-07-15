/**
 * @file dname.c  DNS domain names
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_net.h>
#include <re_dns.h>


#define COMP_MASK   0xc0
#define OFFSET_MASK 0x3fff
#define COMP_LOOP   255


struct dname {
	struct le he;
	size_t pos;
	char *name;
};


static void destructor(void *arg)
{
	struct dname *dn = arg;

	hash_unlink(&dn->he);
	mem_deref(dn->name);
}


static void dname_append(struct hash *ht_dname, const char *name, size_t pos)
{
	struct dname *dn;

	if (!ht_dname || pos > OFFSET_MASK || !*name)
		return;

	dn = mem_zalloc(sizeof(*dn), destructor);
	if (!dn)
		return;

	if (str_dup(&dn->name, name)) {
		mem_deref(dn);
		return;
	}

	hash_append(ht_dname, hash_joaat_str_ci(name), &dn->he, dn);
	dn->pos = pos;
}


static bool lookup_handler(struct le *le, void *arg)
{
	struct dname *dn = le->data;

	return 0 == str_casecmp(dn->name, arg);
}


static inline struct dname *dname_lookup(struct hash *ht_dname,
					 const char *name)
{
	return list_ledata(hash_lookup(ht_dname, hash_joaat_str_ci(name),
				       lookup_handler, (void *)name));
}


static inline int dname_encode_pointer(struct mbuf *mb, size_t pos)
{
	return mbuf_write_u16(mb, htons(pos | (COMP_MASK<<8)));
}


/**
 * Encode a DNS Domain name into a memory buffer
 *
 * @param mb       Memory buffer
 * @param name     Domain name
 * @param ht_dname Domain name hashtable
 * @param start    Start position
 * @param comp     Enable compression
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_dname_encode(struct mbuf *mb, const char *name,
		     struct hash *ht_dname, size_t start, bool comp)
{
	struct dname *dn;
	size_t pos;
	int err;

	if (!mb || !name)
		return EINVAL;

	dn = dname_lookup(ht_dname, name);
	if (dn && comp)
		return dname_encode_pointer(mb, dn->pos);

	pos = mb->pos;
	if (!dn)
		dname_append(ht_dname, name, pos - start);
	err = mbuf_write_u8(mb, 0);

	if ('.' == name[0] && '\0' == name[1])
		return err;

	while (err == 0) {

		const size_t lablen = mb->pos - pos - 1;

		if ('\0' == *name) {
			if (!lablen)
				break;

			mb->buf[pos] = lablen;
			err |= mbuf_write_u8(mb, 0);
			break;
		}
		else if ('.' == *name) {
			if (!lablen)
				return EINVAL;

			mb->buf[pos] = lablen;

			dn = dname_lookup(ht_dname, name + 1);
			if (dn && comp) {
				err |= dname_encode_pointer(mb, dn->pos);
				break;
			}

			pos = mb->pos;
			if (!dn)
				dname_append(ht_dname, name + 1, pos - start);
			err |= mbuf_write_u8(mb, 0);
		}
		else {
			err |= mbuf_write_u8(mb, *name);
		}

		++name;
	}

	return err;
}


/**
 * Decode a DNS domain name from a memory buffer
 *
 * @param mb    Memory buffer to decode from
 * @param name  Pointer to allocated string with domain name
 * @param start Start position
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_dname_decode(struct mbuf *mb, char **name, size_t start)
{
	uint32_t i = 0, loopc = 0;
	bool comp = false;
	size_t pos = 0;
	char buf[256];

	if (!mb || !name)
		return EINVAL;

	while (mb->pos < mb->end) {

		uint8_t len = mb->buf[mb->pos++];
		if (!len) {
			if (comp)
				mb->pos = pos;

			buf[i++] = '\0';

			*name = mem_alloc(i, NULL);
			if (!*name)
				return ENOMEM;

			str_ncpy(*name, buf, i);

			return 0;
		}
		else if ((len & COMP_MASK) == COMP_MASK) {
			uint16_t offset;

			if (loopc++ > COMP_LOOP)
				break;

			--mb->pos;

			offset = ntohs(mbuf_read_u16(mb)) & OFFSET_MASK;
			if (!comp) {
				pos  = mb->pos;
				comp = true;
			}

			mb->pos = offset + start;
			continue;
		}
		else if (len > mbuf_get_left(mb))
			break;
		else if (len > sizeof(buf) - i - 2)
			break;

		if (i > 0)
			buf[i++] = '.';

		while (len--)
			buf[i++] = mb->buf[mb->pos++];
	}

	return EINVAL;
}
