/* <COPYRIGHT_TAG> */

/**
 *\file app_def.h
 *\brief system definition header */

#ifndef _APP_DEF_H_
#define _APP_DEF_H_

typedef enum {
    APP_START_MOD = 0,
    STRM = 0x01,  /* streaming transcoder */
    F2F = 0x02,  /* File2File transcoder */
    FRMW = 0x03,  /* transcoder framework */
    H264D = 0x04,  /* H264 decoder */
    H264E = 0x05,  /* H264 encoder */
    MPEG2D = 0x06,  /* MPEG2 decoder */
    MPEG2E = 0x07,  /* MPEG2 eccoder */
    VC1D = 0x08,  /* VC1 decoder */
    VC1E = 0x09,  /* VC1 eccoder */
    VP8D = 0x0a,  /* VP8 decoder */
    VP8E = 0x0b,  /* VP8 encoder */
    VPP = 0x0c,  /* Video Post Processing */
    MSMT = 0x0d,  /* Measurement */
    TRC = 0x0e,  /* Trace */
    APP = 0x0f,  /* Audio Post Processing */
    APP_MAX_MOD_NUM,
    APP_END_MOD = APP_MAX_MOD_NUM
} AppModId;

typedef enum {
    SYS_ERROR = 1,
    SYS_WARNING,
    SYS_INFORMATION,
    SYS_DEBUG
} TraceLevel;


#ifdef DEBUG
#include <stdio.h>

#define TRC_DEBUG(fmt, ...) \
    do { fprintf(stderr, fmt); } while (0)
#else
#define TRC_DEBUG(fmt, ...)
#endif


#endif

