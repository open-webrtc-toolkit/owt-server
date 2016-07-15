/**
 * @file re_conf.h  Interface to configuration
 *
 * Copyright (C) 2010 Creytiv.com
 */


struct conf;

typedef int (conf_h)(const struct pl *val, void *arg);

int conf_alloc(struct conf **confp, const char *filename);
int conf_alloc_buf(struct conf **confp, const uint8_t *buf, size_t sz);
int conf_get(const struct conf *conf, const char *name, struct pl *pl);
int conf_get_str(const struct conf *conf, const char *name, char *str,
		 size_t size);
int conf_get_u32(const struct conf *conf, const char *name, uint32_t *num);
int conf_get_bool(const struct conf *conf, const char *name, bool *val);
int conf_apply(const struct conf *conf, const char *name,
	       conf_h *ch, void *arg);
