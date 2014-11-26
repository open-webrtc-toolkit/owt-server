/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vpx_ports/mem.h"
#include "vpx_mem/vpx_mem.h"

#include "vp9/decoder/vp9_dboolhuff.h"

int vp9_start_decode(BOOL_DECODER *br,
                     const unsigned char *source,
                     unsigned int source_sz) {
  br->user_buffer_end = source + source_sz;
  br->user_buffer = source;
  br->value = 0;
  br->count = -8;
  br->range = 255;

  if (source_sz && !source)
    return 1;

  /* Populate the buffer */
  vp9_bool_decoder_fill(br);

  return 0;
}


void vp9_bool_decoder_fill(BOOL_DECODER *br) {
  const unsigned char *bufptr = br->user_buffer;
  const unsigned char *bufend = br->user_buffer_end;
  VP9_BD_VALUE value = br->value;
  int count = br->count;
  int shift = VP9_BD_VALUE_SIZE - 8 - (count + 8);
  int loop_end = 0;
  int bits_left = (int)((bufend - bufptr)*CHAR_BIT);
  int x = shift + CHAR_BIT - bits_left;

  if (x >= 0) {
    count += VP9_LOTS_OF_BITS;
    loop_end = x;
  }

  if (x < 0 || bits_left) {
    while (shift >= loop_end) {
      count += CHAR_BIT;
      value |= (VP9_BD_VALUE)*bufptr++ << shift;
      shift -= CHAR_BIT;
    }
  }

  br->user_buffer = bufptr;
  br->value = value;
  br->count = count;
}


static int get_unsigned_bits(unsigned num_values) {
  int cat = 0;
  if (num_values <= 1)
    return 0;
  num_values--;
  while (num_values > 0) {
    cat++;
    num_values >>= 1;
  }
  return cat;
}

int vp9_inv_recenter_nonneg(int v, int m) {
  if (v > (m << 1))
    return v;
  else if ((v & 1) == 0)
    return (v >> 1) + m;
  else
    return m - ((v + 1) >> 1);
}

int vp9_decode_uniform(BOOL_DECODER *br, int n) {
  int v;
  int l = get_unsigned_bits(n);
  int m = (1 << l) - n;
  if (!l) return 0;
  v = decode_value(br, l - 1);
  if (v < m)
    return v;
  else
    return (v << 1) - m + decode_value(br, 1);
}

int vp9_decode_term_subexp(BOOL_DECODER *br, int k, int num_syms) {
  int i = 0, mk = 0, word;
  while (1) {
    int b = (i ? k + i - 1 : k);
    int a = (1 << b);
    if (num_syms <= mk + 3 * a) {
      word = vp9_decode_uniform(br, num_syms - mk) + mk;
      break;
    } else {
      if (decode_value(br, 1)) {
        i++;
        mk += a;
      } else {
        word = decode_value(br, b) + mk;
        break;
      }
    }
  }
  return word;
}

int vp9_decode_unsigned_max(BOOL_DECODER *br, int max) {
  int data = 0, bit = 0, lmax = max;

  while (lmax) {
    data |= decode_bool(br, 128) << bit++;
    lmax >>= 1;
  }
  if (data > max)
    return max;
  return data;
}
