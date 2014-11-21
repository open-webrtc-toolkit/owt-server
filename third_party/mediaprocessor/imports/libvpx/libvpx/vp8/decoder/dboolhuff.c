/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "dboolhuff.h"

int vp8dx_start_decode(BOOL_DECODER *br,
                       const unsigned char *source,
                       unsigned int source_sz,
                       const unsigned char *origin,
                       const unsigned char *key)
{
    br->user_buffer_end = source+source_sz;
    br->user_buffer     = source;
    br->value    = 0;
    br->count    = -8;
    br->range    = 255;
    br->origin = origin;
    br->key = key;

    if (source_sz && !source)
        return 1;

    /* Populate the buffer */
    vp8dx_bool_decoder_fill(br);

    return 0;
}

void vp8dx_bool_decoder_fill(BOOL_DECODER *br)
{
    const unsigned char *bufptr = br->user_buffer;
    const unsigned char *bufend = br->user_buffer_end;
    VP8_BD_VALUE value = br->value;
    int count = br->count;
    int shift = VP8_BD_VALUE_SIZE - 8 - (count + 8);
    size_t bits_left = (bufend - bufptr)*CHAR_BIT;
    int x = (int)(shift + CHAR_BIT - bits_left);
    int loop_end = 0;

    if(x >= 0)
    {
        count += VP8_LOTS_OF_BITS;
        loop_end = x;
    }

    if (x < 0 || bits_left)
    {
        while(shift >= loop_end)
        {
            count += CHAR_BIT;
            value |= ((VP8_BD_VALUE)decrypt_byte(bufptr, br->origin,
                                                 br->key)) << shift;
            ++bufptr;
            shift -= CHAR_BIT;
        }
    }

    br->user_buffer = bufptr;
    br->value = value;
    br->count = count;
}
