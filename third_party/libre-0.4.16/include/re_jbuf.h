/**
 * @file re_jbuf.h  Interface to Jitter Buffer
 *
 * Copyright (C) 2010 Creytiv.com
 */
struct jbuf;
struct rtp_header;

/** Jitter buffer statistics */
struct jbuf_stat {
	uint32_t n_put;        /**< Number of frames put into jitter buffer */
	uint32_t n_get;        /**< Number of frames got from jitter buffer */
	uint32_t n_oos;        /**< Number of out-of-sequence frames        */
	uint32_t n_dups;       /**< Number of duplicate frames detected     */
	uint32_t n_late;       /**< Number of frames arriving too late      */
	uint32_t n_lost;       /**< Number of lost frames                   */
	uint32_t n_overflow;   /**< Number of overflows                     */
	uint32_t n_underflow;  /**< Number of underflows                    */
	uint32_t n_flush;      /**< Number of times jitter buffer flushed   */
};


int  jbuf_alloc(struct jbuf **jbp, uint32_t min, uint32_t max);
int  jbuf_put(struct jbuf *jb, const struct rtp_header *hdr, void *mem);
int  jbuf_get(struct jbuf *jb, struct rtp_header *hdr, void **mem);
void jbuf_flush(struct jbuf *jb);
int  jbuf_stats(const struct jbuf *jb, struct jbuf_stat *jstat);
int  jbuf_debug(struct re_printf *pf, const struct jbuf *jb);
