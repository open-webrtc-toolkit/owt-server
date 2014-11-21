/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
@*INTRODUCTION
 */
#include "vpx_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#define interface (vpx_codec_vp8_dx())
@EXTRA_INCLUDES


#define IVF_FILE_HDR_SZ  (32)
#define IVF_FRAME_HDR_SZ (12)

static unsigned int mem_get_le32(const unsigned char *mem) {
    return (mem[3] << 24)|(mem[2] << 16)|(mem[1] << 8)|(mem[0]);
}

static void die(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    if(fmt[strlen(fmt)-1] != '\n')
        printf("\n");
    exit(EXIT_FAILURE);
}

@DIE_CODEC

@HELPERS

int main(int argc, char **argv) {
    FILE            *infile, *outfile;
    vpx_codec_ctx_t  codec;
    int              flags = 0, frame_cnt = 0;
    unsigned char    file_hdr[IVF_FILE_HDR_SZ];
    unsigned char    frame_hdr[IVF_FRAME_HDR_SZ];
    unsigned char    frame[256*1024];
    vpx_codec_err_t  res;
@@@@EXTRA_VARS

    (void)res;
    /* Open files */
@@@@USAGE
    if(!(infile = fopen(argv[1], "rb")))
        die("Failed to open %s for reading", argv[1]);
    if(!(outfile = fopen(argv[2], "wb")))
        die("Failed to open %s for writing", argv[2]);

    /* Read file header */
    if(!(fread(file_hdr, 1, IVF_FILE_HDR_SZ, infile) == IVF_FILE_HDR_SZ
         && file_hdr[0]=='D' && file_hdr[1]=='K' && file_hdr[2]=='I'
         && file_hdr[3]=='F'))
        die("%s is not an IVF file.", argv[1]);

    printf("Using %s\n",vpx_codec_iface_name(interface));
@@@@DEC_INIT

    /* Read each frame */
    while(fread(frame_hdr, 1, IVF_FRAME_HDR_SZ, infile) == IVF_FRAME_HDR_SZ) {
        int               frame_sz = mem_get_le32(frame_hdr);
        vpx_codec_iter_t  iter = NULL;
        vpx_image_t      *img;


        frame_cnt++;
        if(frame_sz > sizeof(frame))
            die("Frame %d data too big for example code buffer", frame_sz);
        if(fread(frame, 1, frame_sz, infile) != frame_sz)
            die("Frame %d failed to read complete frame", frame_cnt);

@@@@@@@@PRE_DECODE
@@@@@@@@DECODE

        /* Write decoded data to disk */
@@@@@@@@GET_FRAME
            unsigned int plane, y;

@@@@@@@@@@@@PROCESS_DX
        }
    }
    printf("Processed %d frames.\n",frame_cnt);
@@@@DESTROY

    fclose(outfile);
    fclose(infile);
    return EXIT_SUCCESS;
}
