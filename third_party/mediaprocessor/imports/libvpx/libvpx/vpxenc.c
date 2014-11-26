/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vpx_config.h"

#if defined(_WIN32) || defined(__OS2__) || !CONFIG_OS_SUPPORT
#define USE_POSIX_MMAP 0
#else
#define USE_POSIX_MMAP 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "vpx/vpx_encoder.h"
#if CONFIG_DECODERS
#include "vpx/vpx_decoder.h"
#endif
#if USE_POSIX_MMAP
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if CONFIG_VP8_ENCODER || CONFIG_VP9_ENCODER
#include "vpx/vp8cx.h"
#endif
#if CONFIG_VP8_DECODER || CONFIG_VP9_DECODER
#include "vpx/vp8dx.h"
#endif

#include "vpx_ports/mem_ops.h"
#include "vpx_ports/vpx_timer.h"
#include "tools_common.h"
#include "y4minput.h"
#include "libmkv/EbmlWriter.h"
#include "libmkv/EbmlIDs.h"
#include "third_party/libyuv/include/libyuv/scale.h"

/* Need special handling of these functions on Windows */
#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64 */
typedef __int64 off_t;
#define fseeko _fseeki64
#define ftello _ftelli64
#elif defined(_WIN32)
/* MinGW defines off_t as long
   and uses f{seek,tell}o64/off64_t for large files */
#define fseeko fseeko64
#define ftello ftello64
#define off_t off64_t
#endif

#define LITERALU64(hi,lo) ((((uint64_t)hi)<<32)|lo)

/* We should use 32-bit file operations in WebM file format
 * when building ARM executable file (.axf) with RVCT */
#if !CONFIG_OS_SUPPORT
typedef long off_t;
#define fseeko fseek
#define ftello ftell
#endif

/* Swallow warnings about unused results of fread/fwrite */
static size_t wrap_fread(void *ptr, size_t size, size_t nmemb,
                         FILE *stream) {
  return fread(ptr, size, nmemb, stream);
}
#define fread wrap_fread

static size_t wrap_fwrite(const void *ptr, size_t size, size_t nmemb,
                          FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}
#define fwrite wrap_fwrite


static const char *exec_name;

#define VP8_FOURCC (0x30385056)
#define VP9_FOURCC (0x30395056)
static const struct codec_item {
  char const              *name;
  const vpx_codec_iface_t *(*iface)(void);
  const vpx_codec_iface_t *(*dx_iface)(void);
  unsigned int             fourcc;
} codecs[] = {
#if CONFIG_VP8_ENCODER && CONFIG_VP8_DECODER
  {"vp8", &vpx_codec_vp8_cx, &vpx_codec_vp8_dx, VP8_FOURCC},
#elif CONFIG_VP8_ENCODER && !CONFIG_VP8_DECODER
  {"vp8", &vpx_codec_vp8_cx, NULL, VP8_FOURCC},
#endif
#if CONFIG_VP9_ENCODER && CONFIG_VP9_DECODER
  {"vp9", &vpx_codec_vp9_cx, &vpx_codec_vp9_dx, VP9_FOURCC},
#elif CONFIG_VP9_ENCODER && !CONFIG_VP9_DECODER
  {"vp9", &vpx_codec_vp9_cx, NULL, VP9_FOURCC},
#endif
};

static void usage_exit();

#define LOG_ERROR(label) do \
  {\
    const char *l=label;\
    va_list ap;\
    va_start(ap, fmt);\
    if(l)\
      fprintf(stderr, "%s: ", l);\
    vfprintf(stderr, fmt, ap);\
    fprintf(stderr, "\n");\
    va_end(ap);\
  } while(0)

void die(const char *fmt, ...) {
  LOG_ERROR(NULL);
  usage_exit();
}


void fatal(const char *fmt, ...) {
  LOG_ERROR("Fatal");
  exit(EXIT_FAILURE);
}


void warn(const char *fmt, ...) {
  LOG_ERROR("Warning");
}


static void warn_or_exit_on_errorv(vpx_codec_ctx_t *ctx, int fatal,
                                   const char *s, va_list ap) {
  if (ctx->err) {
    const char *detail = vpx_codec_error_detail(ctx);

    vfprintf(stderr, s, ap);
    fprintf(stderr, ": %s\n", vpx_codec_error(ctx));

    if (detail)
      fprintf(stderr, "    %s\n", detail);

    if (fatal)
      exit(EXIT_FAILURE);
  }
}

static void ctx_exit_on_error(vpx_codec_ctx_t *ctx, const char *s, ...) {
  va_list ap;

  va_start(ap, s);
  warn_or_exit_on_errorv(ctx, 1, s, ap);
  va_end(ap);
}

static void warn_or_exit_on_error(vpx_codec_ctx_t *ctx, int fatal,
                                  const char *s, ...) {
  va_list ap;

  va_start(ap, s);
  warn_or_exit_on_errorv(ctx, fatal, s, ap);
  va_end(ap);
}

/* This structure is used to abstract the different ways of handling
 * first pass statistics.
 */
typedef struct {
  vpx_fixed_buf_t buf;
  int             pass;
  FILE           *file;
  char           *buf_ptr;
  size_t          buf_alloc_sz;
} stats_io_t;

int stats_open_file(stats_io_t *stats, const char *fpf, int pass) {
  int res;

  stats->pass = pass;

  if (pass == 0) {
    stats->file = fopen(fpf, "wb");
    stats->buf.sz = 0;
    stats->buf.buf = NULL,
               res = (stats->file != NULL);
  } else {
#if 0
#elif USE_POSIX_MMAP
    struct stat stat_buf;
    int fd;

    fd = open(fpf, O_RDONLY);
    stats->file = fdopen(fd, "rb");
    fstat(fd, &stat_buf);
    stats->buf.sz = stat_buf.st_size;
    stats->buf.buf = mmap(NULL, stats->buf.sz, PROT_READ, MAP_PRIVATE,
                          fd, 0);
    res = (stats->buf.buf != NULL);
#else
    size_t nbytes;

    stats->file = fopen(fpf, "rb");

    if (fseek(stats->file, 0, SEEK_END))
      fatal("First-pass stats file must be seekable!");

    stats->buf.sz = stats->buf_alloc_sz = ftell(stats->file);
    rewind(stats->file);

    stats->buf.buf = malloc(stats->buf_alloc_sz);

    if (!stats->buf.buf)
      fatal("Failed to allocate first-pass stats buffer (%lu bytes)",
            (unsigned long)stats->buf_alloc_sz);

    nbytes = fread(stats->buf.buf, 1, stats->buf.sz, stats->file);
    res = (nbytes == stats->buf.sz);
#endif
  }

  return res;
}

int stats_open_mem(stats_io_t *stats, int pass) {
  int res;
  stats->pass = pass;

  if (!pass) {
    stats->buf.sz = 0;
    stats->buf_alloc_sz = 64 * 1024;
    stats->buf.buf = malloc(stats->buf_alloc_sz);
  }

  stats->buf_ptr = stats->buf.buf;
  res = (stats->buf.buf != NULL);
  return res;
}


void stats_close(stats_io_t *stats, int last_pass) {
  if (stats->file) {
    if (stats->pass == last_pass) {
#if 0
#elif USE_POSIX_MMAP
      munmap(stats->buf.buf, stats->buf.sz);
#else
      free(stats->buf.buf);
#endif
    }

    fclose(stats->file);
    stats->file = NULL;
  } else {
    if (stats->pass == last_pass)
      free(stats->buf.buf);
  }
}

void stats_write(stats_io_t *stats, const void *pkt, size_t len) {
  if (stats->file) {
    (void) fwrite(pkt, 1, len, stats->file);
  } else {
    if (stats->buf.sz + len > stats->buf_alloc_sz) {
      size_t  new_sz = stats->buf_alloc_sz + 64 * 1024;
      char   *new_ptr = realloc(stats->buf.buf, new_sz);

      if (new_ptr) {
        stats->buf_ptr = new_ptr + (stats->buf_ptr - (char *)stats->buf.buf);
        stats->buf.buf = new_ptr;
        stats->buf_alloc_sz = new_sz;
      } else
        fatal("Failed to realloc firstpass stats buffer.");
    }

    memcpy(stats->buf_ptr, pkt, len);
    stats->buf.sz += len;
    stats->buf_ptr += len;
  }
}

vpx_fixed_buf_t stats_get(stats_io_t *stats) {
  return stats->buf;
}

/* Stereo 3D packed frame format */
typedef enum stereo_format {
  STEREO_FORMAT_MONO       = 0,
  STEREO_FORMAT_LEFT_RIGHT = 1,
  STEREO_FORMAT_BOTTOM_TOP = 2,
  STEREO_FORMAT_TOP_BOTTOM = 3,
  STEREO_FORMAT_RIGHT_LEFT = 11
} stereo_format_t;

enum video_file_type {
  FILE_TYPE_RAW,
  FILE_TYPE_IVF,
  FILE_TYPE_Y4M
};

struct detect_buffer {
  char buf[4];
  size_t buf_read;
  size_t position;
};


struct input_state {
  char                 *fn;
  FILE                 *file;
  off_t                 length;
  y4m_input             y4m;
  struct detect_buffer  detect;
  enum video_file_type  file_type;
  unsigned int          w;
  unsigned int          h;
  struct vpx_rational   framerate;
  int                   use_i420;
};


#define IVF_FRAME_HDR_SZ (4+8) /* 4 byte size + 8 byte timestamp */
static int read_frame(struct input_state *input, vpx_image_t *img) {
  FILE *f = input->file;
  enum video_file_type file_type = input->file_type;
  y4m_input *y4m = &input->y4m;
  struct detect_buffer *detect = &input->detect;
  int plane = 0;
  int shortread = 0;

  if (file_type == FILE_TYPE_Y4M) {
    if (y4m_input_fetch_frame(y4m, f, img) < 1)
      return 0;
  } else {
    if (file_type == FILE_TYPE_IVF) {
      char junk[IVF_FRAME_HDR_SZ];

      /* Skip the frame header. We know how big the frame should be. See
       * write_ivf_frame_header() for documentation on the frame header
       * layout.
       */
      (void) fread(junk, 1, IVF_FRAME_HDR_SZ, f);
    }

    for (plane = 0; plane < 3; plane++) {
      unsigned char *ptr;
      int w = (plane ? (1 + img->d_w) / 2 : img->d_w);
      int h = (plane ? (1 + img->d_h) / 2 : img->d_h);
      int r;

      /* Determine the correct plane based on the image format. The for-loop
       * always counts in Y,U,V order, but this may not match the order of
       * the data on disk.
       */
      switch (plane) {
        case 1:
          ptr = img->planes[img->fmt == VPX_IMG_FMT_YV12 ? VPX_PLANE_V : VPX_PLANE_U];
          break;
        case 2:
          ptr = img->planes[img->fmt == VPX_IMG_FMT_YV12 ? VPX_PLANE_U : VPX_PLANE_V];
          break;
        default:
          ptr = img->planes[plane];
      }

      for (r = 0; r < h; r++) {
        size_t needed = w;
        size_t buf_position = 0;
        const size_t left = detect->buf_read - detect->position;
        if (left > 0) {
          const size_t more = (left < needed) ? left : needed;
          memcpy(ptr, detect->buf + detect->position, more);
          buf_position = more;
          needed -= more;
          detect->position += more;
        }
        if (needed > 0) {
          shortread |= (fread(ptr + buf_position, 1, needed, f) < needed);
        }

        ptr += img->stride[plane];
      }
    }
  }

  return !shortread;
}


unsigned int file_is_y4m(FILE      *infile,
                         y4m_input *y4m,
                         char       detect[4]) {
  if (memcmp(detect, "YUV4", 4) == 0) {
    return 1;
  }
  return 0;
}

#define IVF_FILE_HDR_SZ (32)
unsigned int file_is_ivf(struct input_state *input,
                         unsigned int *fourcc) {
  char raw_hdr[IVF_FILE_HDR_SZ];
  int is_ivf = 0;
  FILE *infile = input->file;
  unsigned int *width = &input->w;
  unsigned int *height = &input->h;
  struct detect_buffer *detect = &input->detect;

  if (memcmp(detect->buf, "DKIF", 4) != 0)
    return 0;

  /* See write_ivf_file_header() for more documentation on the file header
   * layout.
   */
  if (fread(raw_hdr + 4, 1, IVF_FILE_HDR_SZ - 4, infile)
      == IVF_FILE_HDR_SZ - 4) {
    {
      is_ivf = 1;

      if (mem_get_le16(raw_hdr + 4) != 0)
        warn("Unrecognized IVF version! This file may not decode "
             "properly.");

      *fourcc = mem_get_le32(raw_hdr + 8);
    }
  }

  if (is_ivf) {
    *width = mem_get_le16(raw_hdr + 12);
    *height = mem_get_le16(raw_hdr + 14);
    detect->position = 4;
  }

  return is_ivf;
}


static void write_ivf_file_header(FILE *outfile,
                                  const vpx_codec_enc_cfg_t *cfg,
                                  unsigned int fourcc,
                                  int frame_cnt) {
  char header[32];

  if (cfg->g_pass != VPX_RC_ONE_PASS && cfg->g_pass != VPX_RC_LAST_PASS)
    return;

  header[0] = 'D';
  header[1] = 'K';
  header[2] = 'I';
  header[3] = 'F';
  mem_put_le16(header + 4,  0);                 /* version */
  mem_put_le16(header + 6,  32);                /* headersize */
  mem_put_le32(header + 8,  fourcc);            /* headersize */
  mem_put_le16(header + 12, cfg->g_w);          /* width */
  mem_put_le16(header + 14, cfg->g_h);          /* height */
  mem_put_le32(header + 16, cfg->g_timebase.den); /* rate */
  mem_put_le32(header + 20, cfg->g_timebase.num); /* scale */
  mem_put_le32(header + 24, frame_cnt);         /* length */
  mem_put_le32(header + 28, 0);                 /* unused */

  (void) fwrite(header, 1, 32, outfile);
}


static void write_ivf_frame_header(FILE *outfile,
                                   const vpx_codec_cx_pkt_t *pkt) {
  char             header[12];
  vpx_codec_pts_t  pts;

  if (pkt->kind != VPX_CODEC_CX_FRAME_PKT)
    return;

  pts = pkt->data.frame.pts;
  mem_put_le32(header, (int)pkt->data.frame.sz);
  mem_put_le32(header + 4, pts & 0xFFFFFFFF);
  mem_put_le32(header + 8, pts >> 32);

  (void) fwrite(header, 1, 12, outfile);
}

static void write_ivf_frame_size(FILE *outfile, size_t size) {
  char             header[4];
  mem_put_le32(header, (int)size);
  (void) fwrite(header, 1, 4, outfile);
}


typedef off_t EbmlLoc;


struct cue_entry {
  unsigned int time;
  uint64_t     loc;
};


struct EbmlGlobal {
  int debug;

  FILE    *stream;
  int64_t last_pts_ms;
  vpx_rational_t  framerate;

  /* These pointers are to the start of an element */
  off_t    position_reference;
  off_t    seek_info_pos;
  off_t    segment_info_pos;
  off_t    track_pos;
  off_t    cue_pos;
  off_t    cluster_pos;

  /* This pointer is to a specific element to be serialized */
  off_t    track_id_pos;

  /* These pointers are to the size field of the element */
  EbmlLoc  startSegment;
  EbmlLoc  startCluster;

  uint32_t cluster_timecode;
  int      cluster_open;

  struct cue_entry *cue_list;
  unsigned int      cues;

};


void Ebml_Write(EbmlGlobal *glob, const void *buffer_in, unsigned long len) {
  (void) fwrite(buffer_in, 1, len, glob->stream);
}

#define WRITE_BUFFER(s) \
  for(i = len-1; i>=0; i--)\
  { \
    x = (char)(*(const s *)buffer_in >> (i * CHAR_BIT)); \
    Ebml_Write(glob, &x, 1); \
  }
void Ebml_Serialize(EbmlGlobal *glob, const void *buffer_in, int buffer_size, unsigned long len) {
  char x;
  int i;

  /* buffer_size:
   * 1 - int8_t;
   * 2 - int16_t;
   * 3 - int32_t;
   * 4 - int64_t;
   */
  switch (buffer_size) {
    case 1:
      WRITE_BUFFER(int8_t)
      break;
    case 2:
      WRITE_BUFFER(int16_t)
      break;
    case 4:
      WRITE_BUFFER(int32_t)
      break;
    case 8:
      WRITE_BUFFER(int64_t)
      break;
    default:
      break;
  }
}
#undef WRITE_BUFFER

/* Need a fixed size serializer for the track ID. libmkv provides a 64 bit
 * one, but not a 32 bit one.
 */
static void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned long class_id, uint64_t ui) {
  unsigned char sizeSerialized = 4 | 0x80;
  Ebml_WriteID(glob, class_id);
  Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
  Ebml_Serialize(glob, &ui, sizeof(ui), 4);
}


static void
Ebml_StartSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc,
                     unsigned long class_id) {
  /* todo this is always taking 8 bytes, this may need later optimization */
  /* this is a key that says length unknown */
  uint64_t unknownLen = LITERALU64(0x01FFFFFF, 0xFFFFFFFF);

  Ebml_WriteID(glob, class_id);
  *ebmlLoc = ftello(glob->stream);
  Ebml_Serialize(glob, &unknownLen, sizeof(unknownLen), 8);
}

static void
Ebml_EndSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc) {
  off_t pos;
  uint64_t size;

  /* Save the current stream pointer */
  pos = ftello(glob->stream);

  /* Calculate the size of this element */
  size = pos - *ebmlLoc - 8;
  size |= LITERALU64(0x01000000, 0x00000000);

  /* Seek back to the beginning of the element and write the new size */
  fseeko(glob->stream, *ebmlLoc, SEEK_SET);
  Ebml_Serialize(glob, &size, sizeof(size), 8);

  /* Reset the stream pointer */
  fseeko(glob->stream, pos, SEEK_SET);
}


static void
write_webm_seek_element(EbmlGlobal *ebml, unsigned long id, off_t pos) {
  uint64_t offset = pos - ebml->position_reference;
  EbmlLoc start;
  Ebml_StartSubElement(ebml, &start, Seek);
  Ebml_SerializeBinary(ebml, SeekID, id);
  Ebml_SerializeUnsigned64(ebml, SeekPosition, offset);
  Ebml_EndSubElement(ebml, &start);
}


static void
write_webm_seek_info(EbmlGlobal *ebml) {

  off_t pos;

  /* Save the current stream pointer */
  pos = ftello(ebml->stream);

  if (ebml->seek_info_pos)
    fseeko(ebml->stream, ebml->seek_info_pos, SEEK_SET);
  else
    ebml->seek_info_pos = pos;

  {
    EbmlLoc start;

    Ebml_StartSubElement(ebml, &start, SeekHead);
    write_webm_seek_element(ebml, Tracks, ebml->track_pos);
    write_webm_seek_element(ebml, Cues,   ebml->cue_pos);
    write_webm_seek_element(ebml, Info,   ebml->segment_info_pos);
    Ebml_EndSubElement(ebml, &start);
  }
  {
    /* segment info */
    EbmlLoc startInfo;
    uint64_t frame_time;
    char version_string[64];

    /* Assemble version string */
    if (ebml->debug)
      strcpy(version_string, "vpxenc");
    else {
      strcpy(version_string, "vpxenc ");
      strncat(version_string,
              vpx_codec_version_str(),
              sizeof(version_string) - 1 - strlen(version_string));
    }

    frame_time = (uint64_t)1000 * ebml->framerate.den
                 / ebml->framerate.num;
    ebml->segment_info_pos = ftello(ebml->stream);
    Ebml_StartSubElement(ebml, &startInfo, Info);
    Ebml_SerializeUnsigned(ebml, TimecodeScale, 1000000);
    Ebml_SerializeFloat(ebml, Segment_Duration,
                        (double)(ebml->last_pts_ms + frame_time));
    Ebml_SerializeString(ebml, 0x4D80, version_string);
    Ebml_SerializeString(ebml, 0x5741, version_string);
    Ebml_EndSubElement(ebml, &startInfo);
  }
}


static void
write_webm_file_header(EbmlGlobal                *glob,
                       const vpx_codec_enc_cfg_t *cfg,
                       const struct vpx_rational *fps,
                       stereo_format_t            stereo_fmt,
                       unsigned int               fourcc) {
  {
    EbmlLoc start;
    Ebml_StartSubElement(glob, &start, EBML);
    Ebml_SerializeUnsigned(glob, EBMLVersion, 1);
    Ebml_SerializeUnsigned(glob, EBMLReadVersion, 1);
    Ebml_SerializeUnsigned(glob, EBMLMaxIDLength, 4);
    Ebml_SerializeUnsigned(glob, EBMLMaxSizeLength, 8);
    Ebml_SerializeString(glob, DocType, "webm");
    Ebml_SerializeUnsigned(glob, DocTypeVersion, 2);
    Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 2);
    Ebml_EndSubElement(glob, &start);
  }
  {
    Ebml_StartSubElement(glob, &glob->startSegment, Segment);
    glob->position_reference = ftello(glob->stream);
    glob->framerate = *fps;
    write_webm_seek_info(glob);

    {
      EbmlLoc trackStart;
      glob->track_pos = ftello(glob->stream);
      Ebml_StartSubElement(glob, &trackStart, Tracks);
      {
        unsigned int trackNumber = 1;
        uint64_t     trackID = 0;

        EbmlLoc start;
        Ebml_StartSubElement(glob, &start, TrackEntry);
        Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
        glob->track_id_pos = ftello(glob->stream);
        Ebml_SerializeUnsigned32(glob, TrackUID, trackID);
        Ebml_SerializeUnsigned(glob, TrackType, 1);
        Ebml_SerializeString(glob, CodecID,
                             fourcc == VP8_FOURCC ? "V_VP8" : "V_VP9");
        {
          unsigned int pixelWidth = cfg->g_w;
          unsigned int pixelHeight = cfg->g_h;
          float        frameRate   = (float)fps->num / (float)fps->den;

          EbmlLoc videoStart;
          Ebml_StartSubElement(glob, &videoStart, Video);
          Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
          Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
          Ebml_SerializeUnsigned(glob, StereoMode, stereo_fmt);
          Ebml_SerializeFloat(glob, FrameRate, frameRate);
          Ebml_EndSubElement(glob, &videoStart);
        }
        Ebml_EndSubElement(glob, &start); /* Track Entry */
      }
      Ebml_EndSubElement(glob, &trackStart);
    }
    /* segment element is open */
  }
}


static void
write_webm_block(EbmlGlobal                *glob,
                 const vpx_codec_enc_cfg_t *cfg,
                 const vpx_codec_cx_pkt_t  *pkt) {
  unsigned long  block_length;
  unsigned char  track_number;
  unsigned short block_timecode = 0;
  unsigned char  flags;
  int64_t        pts_ms;
  int            start_cluster = 0, is_keyframe;

  /* Calculate the PTS of this frame in milliseconds */
  pts_ms = pkt->data.frame.pts * 1000
           * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;
  if (pts_ms <= glob->last_pts_ms)
    pts_ms = glob->last_pts_ms + 1;
  glob->last_pts_ms = pts_ms;

  /* Calculate the relative time of this block */
  if (pts_ms - glob->cluster_timecode > SHRT_MAX)
    start_cluster = 1;
  else
    block_timecode = (unsigned short)pts_ms - glob->cluster_timecode;

  is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY);
  if (start_cluster || is_keyframe) {
    if (glob->cluster_open)
      Ebml_EndSubElement(glob, &glob->startCluster);

    /* Open the new cluster */
    block_timecode = 0;
    glob->cluster_open = 1;
    glob->cluster_timecode = (uint32_t)pts_ms;
    glob->cluster_pos = ftello(glob->stream);
    Ebml_StartSubElement(glob, &glob->startCluster, Cluster); /* cluster */
    Ebml_SerializeUnsigned(glob, Timecode, glob->cluster_timecode);

    /* Save a cue point if this is a keyframe. */
    if (is_keyframe) {
      struct cue_entry *cue, *new_cue_list;

      new_cue_list = realloc(glob->cue_list,
                             (glob->cues + 1) * sizeof(struct cue_entry));
      if (new_cue_list)
        glob->cue_list = new_cue_list;
      else
        fatal("Failed to realloc cue list.");

      cue = &glob->cue_list[glob->cues];
      cue->time = glob->cluster_timecode;
      cue->loc = glob->cluster_pos;
      glob->cues++;
    }
  }

  /* Write the Simple Block */
  Ebml_WriteID(glob, SimpleBlock);

  block_length = (unsigned long)pkt->data.frame.sz + 4;
  block_length |= 0x10000000;
  Ebml_Serialize(glob, &block_length, sizeof(block_length), 4);

  track_number = 1;
  track_number |= 0x80;
  Ebml_Write(glob, &track_number, 1);

  Ebml_Serialize(glob, &block_timecode, sizeof(block_timecode), 2);

  flags = 0;
  if (is_keyframe)
    flags |= 0x80;
  if (pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
    flags |= 0x08;
  Ebml_Write(glob, &flags, 1);

  Ebml_Write(glob, pkt->data.frame.buf, (unsigned long)pkt->data.frame.sz);
}


static void
write_webm_file_footer(EbmlGlobal *glob, long hash) {

  if (glob->cluster_open)
    Ebml_EndSubElement(glob, &glob->startCluster);

  {
    EbmlLoc start;
    unsigned int i;

    glob->cue_pos = ftello(glob->stream);
    Ebml_StartSubElement(glob, &start, Cues);
    for (i = 0; i < glob->cues; i++) {
      struct cue_entry *cue = &glob->cue_list[i];
      EbmlLoc start;

      Ebml_StartSubElement(glob, &start, CuePoint);
      {
        EbmlLoc start;

        Ebml_SerializeUnsigned(glob, CueTime, cue->time);

        Ebml_StartSubElement(glob, &start, CueTrackPositions);
        Ebml_SerializeUnsigned(glob, CueTrack, 1);
        Ebml_SerializeUnsigned64(glob, CueClusterPosition,
                                 cue->loc - glob->position_reference);
        Ebml_EndSubElement(glob, &start);
      }
      Ebml_EndSubElement(glob, &start);
    }
    Ebml_EndSubElement(glob, &start);
  }

  Ebml_EndSubElement(glob, &glob->startSegment);

  /* Patch up the seek info block */
  write_webm_seek_info(glob);

  /* Patch up the track id */
  fseeko(glob->stream, glob->track_id_pos, SEEK_SET);
  Ebml_SerializeUnsigned32(glob, TrackUID, glob->debug ? 0xDEADBEEF : hash);

  fseeko(glob->stream, 0, SEEK_END);
}


/* Murmur hash derived from public domain reference implementation at
 *   http:// sites.google.com/site/murmurhash/
 */
static unsigned int murmur(const void *key, int len, unsigned int seed) {
  const unsigned int m = 0x5bd1e995;
  const int r = 24;

  unsigned int h = seed ^ len;

  const unsigned char *data = (const unsigned char *)key;

  while (len >= 4) {
    unsigned int k;

    k  = data[0];
    k |= data[1] << 8;
    k |= data[2] << 16;
    k |= data[3] << 24;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  switch (len) {
    case 3:
      h ^= data[2] << 16;
    case 2:
      h ^= data[1] << 8;
    case 1:
      h ^= data[0];
      h *= m;
  };

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#include "math.h"
#define MAX_PSNR 100
static double vp8_mse2psnr(double Samples, double Peak, double Mse) {
  double psnr;

  if ((double)Mse > 0.0)
    psnr = 10.0 * log10(Peak * Peak * Samples / Mse);
  else
    psnr = MAX_PSNR;      /* Limit to prevent / 0 */

  if (psnr > MAX_PSNR)
    psnr = MAX_PSNR;

  return psnr;
}


#include "args.h"
static const arg_def_t debugmode = ARG_DEF("D", "debug", 0,
                                           "Debug mode (makes output deterministic)");
static const arg_def_t outputfile = ARG_DEF("o", "output", 1,
                                            "Output filename");
static const arg_def_t use_yv12 = ARG_DEF(NULL, "yv12", 0,
                                          "Input file is YV12 ");
static const arg_def_t use_i420 = ARG_DEF(NULL, "i420", 0,
                                          "Input file is I420 (default)");
static const arg_def_t codecarg = ARG_DEF(NULL, "codec", 1,
                                          "Codec to use");
static const arg_def_t passes           = ARG_DEF("p", "passes", 1,
                                                  "Number of passes (1/2)");
static const arg_def_t pass_arg         = ARG_DEF(NULL, "pass", 1,
                                                  "Pass to execute (1/2)");
static const arg_def_t fpf_name         = ARG_DEF(NULL, "fpf", 1,
                                                  "First pass statistics file name");
static const arg_def_t limit = ARG_DEF(NULL, "limit", 1,
                                       "Stop encoding after n input frames");
static const arg_def_t skip = ARG_DEF(NULL, "skip", 1,
                                      "Skip the first n input frames");
static const arg_def_t deadline         = ARG_DEF("d", "deadline", 1,
                                                  "Deadline per frame (usec)");
static const arg_def_t best_dl          = ARG_DEF(NULL, "best", 0,
                                                  "Use Best Quality Deadline");
static const arg_def_t good_dl          = ARG_DEF(NULL, "good", 0,
                                                  "Use Good Quality Deadline");
static const arg_def_t rt_dl            = ARG_DEF(NULL, "rt", 0,
                                                  "Use Realtime Quality Deadline");
static const arg_def_t quietarg         = ARG_DEF("q", "quiet", 0,
                                                  "Do not print encode progress");
static const arg_def_t verbosearg       = ARG_DEF("v", "verbose", 0,
                                                  "Show encoder parameters");
static const arg_def_t psnrarg          = ARG_DEF(NULL, "psnr", 0,
                                                  "Show PSNR in status line");
enum TestDecodeFatality {
  TEST_DECODE_OFF,
  TEST_DECODE_FATAL,
  TEST_DECODE_WARN,
};
static const struct arg_enum_list test_decode_enum[] = {
  {"off",   TEST_DECODE_OFF},
  {"fatal", TEST_DECODE_FATAL},
  {"warn",  TEST_DECODE_WARN},
  {NULL, 0}
};
static const arg_def_t recontest = ARG_DEF_ENUM(NULL, "test-decode", 1,
                                                "Test encode/decode mismatch",
                                                test_decode_enum);
static const arg_def_t framerate        = ARG_DEF(NULL, "fps", 1,
                                                  "Stream frame rate (rate/scale)");
static const arg_def_t use_ivf          = ARG_DEF(NULL, "ivf", 0,
                                                  "Output IVF (default is WebM)");
static const arg_def_t out_part = ARG_DEF("P", "output-partitions", 0,
                                          "Makes encoder output partitions. Requires IVF output!");
static const arg_def_t q_hist_n         = ARG_DEF(NULL, "q-hist", 1,
                                                  "Show quantizer histogram (n-buckets)");
static const arg_def_t rate_hist_n         = ARG_DEF(NULL, "rate-hist", 1,
                                                     "Show rate histogram (n-buckets)");
static const arg_def_t *main_args[] = {
  &debugmode,
  &outputfile, &codecarg, &passes, &pass_arg, &fpf_name, &limit, &skip,
  &deadline, &best_dl, &good_dl, &rt_dl,
  &quietarg, &verbosearg, &psnrarg, &use_ivf, &out_part, &q_hist_n, &rate_hist_n,
  NULL
};

static const arg_def_t usage            = ARG_DEF("u", "usage", 1,
                                                  "Usage profile number to use");
static const arg_def_t threads          = ARG_DEF("t", "threads", 1,
                                                  "Max number of threads to use");
static const arg_def_t profile          = ARG_DEF(NULL, "profile", 1,
                                                  "Bitstream profile number to use");
static const arg_def_t width            = ARG_DEF("w", "width", 1,
                                                  "Frame width");
static const arg_def_t height           = ARG_DEF("h", "height", 1,
                                                  "Frame height");
static const struct arg_enum_list stereo_mode_enum[] = {
  {"mono", STEREO_FORMAT_MONO},
  {"left-right", STEREO_FORMAT_LEFT_RIGHT},
  {"bottom-top", STEREO_FORMAT_BOTTOM_TOP},
  {"top-bottom", STEREO_FORMAT_TOP_BOTTOM},
  {"right-left", STEREO_FORMAT_RIGHT_LEFT},
  {NULL, 0}
};
static const arg_def_t stereo_mode      = ARG_DEF_ENUM(NULL, "stereo-mode", 1,
                                                       "Stereo 3D video format", stereo_mode_enum);
static const arg_def_t timebase         = ARG_DEF(NULL, "timebase", 1,
                                                  "Output timestamp precision (fractional seconds)");
static const arg_def_t error_resilient  = ARG_DEF(NULL, "error-resilient", 1,
                                                  "Enable error resiliency features");
static const arg_def_t lag_in_frames    = ARG_DEF(NULL, "lag-in-frames", 1,
                                                  "Max number of frames to lag");

static const arg_def_t *global_args[] = {
  &use_yv12, &use_i420, &usage, &threads, &profile,
  &width, &height, &stereo_mode, &timebase, &framerate,
  &error_resilient,
  &lag_in_frames, NULL
};

static const arg_def_t dropframe_thresh   = ARG_DEF(NULL, "drop-frame", 1,
                                                    "Temporal resampling threshold (buf %)");
static const arg_def_t resize_allowed     = ARG_DEF(NULL, "resize-allowed", 1,
                                                    "Spatial resampling enabled (bool)");
static const arg_def_t resize_up_thresh   = ARG_DEF(NULL, "resize-up", 1,
                                                    "Upscale threshold (buf %)");
static const arg_def_t resize_down_thresh = ARG_DEF(NULL, "resize-down", 1,
                                                    "Downscale threshold (buf %)");
static const struct arg_enum_list end_usage_enum[] = {
  {"vbr", VPX_VBR},
  {"cbr", VPX_CBR},
  {"cq",  VPX_CQ},
  {NULL, 0}
};
static const arg_def_t end_usage          = ARG_DEF_ENUM(NULL, "end-usage", 1,
                                                         "Rate control mode", end_usage_enum);
static const arg_def_t target_bitrate     = ARG_DEF(NULL, "target-bitrate", 1,
                                                    "Bitrate (kbps)");
static const arg_def_t min_quantizer      = ARG_DEF(NULL, "min-q", 1,
                                                    "Minimum (best) quantizer");
static const arg_def_t max_quantizer      = ARG_DEF(NULL, "max-q", 1,
                                                    "Maximum (worst) quantizer");
static const arg_def_t undershoot_pct     = ARG_DEF(NULL, "undershoot-pct", 1,
                                                    "Datarate undershoot (min) target (%)");
static const arg_def_t overshoot_pct      = ARG_DEF(NULL, "overshoot-pct", 1,
                                                    "Datarate overshoot (max) target (%)");
static const arg_def_t buf_sz             = ARG_DEF(NULL, "buf-sz", 1,
                                                    "Client buffer size (ms)");
static const arg_def_t buf_initial_sz     = ARG_DEF(NULL, "buf-initial-sz", 1,
                                                    "Client initial buffer size (ms)");
static const arg_def_t buf_optimal_sz     = ARG_DEF(NULL, "buf-optimal-sz", 1,
                                                    "Client optimal buffer size (ms)");
static const arg_def_t *rc_args[] = {
  &dropframe_thresh, &resize_allowed, &resize_up_thresh, &resize_down_thresh,
  &end_usage, &target_bitrate, &min_quantizer, &max_quantizer,
  &undershoot_pct, &overshoot_pct, &buf_sz, &buf_initial_sz, &buf_optimal_sz,
  NULL
};


static const arg_def_t bias_pct = ARG_DEF(NULL, "bias-pct", 1,
                                          "CBR/VBR bias (0=CBR, 100=VBR)");
static const arg_def_t minsection_pct = ARG_DEF(NULL, "minsection-pct", 1,
                                                "GOP min bitrate (% of target)");
static const arg_def_t maxsection_pct = ARG_DEF(NULL, "maxsection-pct", 1,
                                                "GOP max bitrate (% of target)");
static const arg_def_t *rc_twopass_args[] = {
  &bias_pct, &minsection_pct, &maxsection_pct, NULL
};


static const arg_def_t kf_min_dist = ARG_DEF(NULL, "kf-min-dist", 1,
                                             "Minimum keyframe interval (frames)");
static const arg_def_t kf_max_dist = ARG_DEF(NULL, "kf-max-dist", 1,
                                             "Maximum keyframe interval (frames)");
static const arg_def_t kf_disabled = ARG_DEF(NULL, "disable-kf", 0,
                                             "Disable keyframe placement");
static const arg_def_t *kf_args[] = {
  &kf_min_dist, &kf_max_dist, &kf_disabled, NULL
};


static const arg_def_t noise_sens = ARG_DEF(NULL, "noise-sensitivity", 1,
                                            "Noise sensitivity (frames to blur)");
static const arg_def_t sharpness = ARG_DEF(NULL, "sharpness", 1,
                                           "Filter sharpness (0-7)");
static const arg_def_t static_thresh = ARG_DEF(NULL, "static-thresh", 1,
                                               "Motion detection threshold");
static const arg_def_t cpu_used = ARG_DEF(NULL, "cpu-used", 1,
                                          "CPU Used (-16..16)");
static const arg_def_t token_parts = ARG_DEF(NULL, "token-parts", 1,
                                     "Number of token partitions to use, log2");
static const arg_def_t tile_cols = ARG_DEF(NULL, "tile-columns", 1,
                                         "Number of tile columns to use, log2");
static const arg_def_t tile_rows = ARG_DEF(NULL, "tile-rows", 1,
                                           "Number of tile rows to use, log2");
static const arg_def_t auto_altref = ARG_DEF(NULL, "auto-alt-ref", 1,
                                             "Enable automatic alt reference frames");
static const arg_def_t arnr_maxframes = ARG_DEF(NULL, "arnr-maxframes", 1,
                                                "AltRef Max Frames");
static const arg_def_t arnr_strength = ARG_DEF(NULL, "arnr-strength", 1,
                                               "AltRef Strength");
static const arg_def_t arnr_type = ARG_DEF(NULL, "arnr-type", 1,
                                           "AltRef Type");
static const struct arg_enum_list tuning_enum[] = {
  {"psnr", VP8_TUNE_PSNR},
  {"ssim", VP8_TUNE_SSIM},
  {NULL, 0}
};
static const arg_def_t tune_ssim = ARG_DEF_ENUM(NULL, "tune", 1,
                                                "Material to favor", tuning_enum);
static const arg_def_t cq_level = ARG_DEF(NULL, "cq-level", 1,
                                          "Constrained Quality Level");
static const arg_def_t max_intra_rate_pct = ARG_DEF(NULL, "max-intra-rate", 1,
                                                    "Max I-frame bitrate (pct)");
static const arg_def_t lossless = ARG_DEF(NULL, "lossless", 1, "Lossless mode");
#if CONFIG_VP9_ENCODER
static const arg_def_t frame_parallel_decoding  = ARG_DEF(
    NULL, "frame-parallel", 1, "Enable frame parallel decodability features");
#endif

#if CONFIG_VP8_ENCODER
static const arg_def_t *vp8_args[] = {
  &cpu_used, &auto_altref, &noise_sens, &sharpness, &static_thresh,
  &token_parts, &arnr_maxframes, &arnr_strength, &arnr_type,
  &tune_ssim, &cq_level, &max_intra_rate_pct,
  NULL
};
static const int vp8_arg_ctrl_map[] = {
  VP8E_SET_CPUUSED, VP8E_SET_ENABLEAUTOALTREF,
  VP8E_SET_NOISE_SENSITIVITY, VP8E_SET_SHARPNESS, VP8E_SET_STATIC_THRESHOLD,
  VP8E_SET_TOKEN_PARTITIONS,
  VP8E_SET_ARNR_MAXFRAMES, VP8E_SET_ARNR_STRENGTH, VP8E_SET_ARNR_TYPE,
  VP8E_SET_TUNING, VP8E_SET_CQ_LEVEL, VP8E_SET_MAX_INTRA_BITRATE_PCT,
  0
};
#endif

#if CONFIG_VP9_ENCODER
static const arg_def_t *vp9_args[] = {
  &cpu_used, &auto_altref, &noise_sens, &sharpness, &static_thresh,
  &tile_cols, &tile_rows, &arnr_maxframes, &arnr_strength, &arnr_type,
  &tune_ssim, &cq_level, &max_intra_rate_pct, &lossless,
  &frame_parallel_decoding,
  NULL
};
static const int vp9_arg_ctrl_map[] = {
  VP8E_SET_CPUUSED, VP8E_SET_ENABLEAUTOALTREF,
  VP8E_SET_NOISE_SENSITIVITY, VP8E_SET_SHARPNESS, VP8E_SET_STATIC_THRESHOLD,
  VP9E_SET_TILE_COLUMNS, VP9E_SET_TILE_ROWS,
  VP8E_SET_ARNR_MAXFRAMES, VP8E_SET_ARNR_STRENGTH, VP8E_SET_ARNR_TYPE,
  VP8E_SET_TUNING, VP8E_SET_CQ_LEVEL, VP8E_SET_MAX_INTRA_BITRATE_PCT,
  VP9E_SET_LOSSLESS, VP9E_SET_FRAME_PARALLEL_DECODING,
  0
};
#endif

static const arg_def_t *no_args[] = { NULL };

static void usage_exit() {
  int i;

  fprintf(stderr, "Usage: %s <options> -o dst_filename src_filename \n",
          exec_name);

  fprintf(stderr, "\nOptions:\n");
  arg_show_usage(stdout, main_args);
  fprintf(stderr, "\nEncoder Global Options:\n");
  arg_show_usage(stdout, global_args);
  fprintf(stderr, "\nRate Control Options:\n");
  arg_show_usage(stdout, rc_args);
  fprintf(stderr, "\nTwopass Rate Control Options:\n");
  arg_show_usage(stdout, rc_twopass_args);
  fprintf(stderr, "\nKeyframe Placement Options:\n");
  arg_show_usage(stdout, kf_args);
#if CONFIG_VP8_ENCODER
  fprintf(stderr, "\nVP8 Specific Options:\n");
  arg_show_usage(stdout, vp8_args);
#endif
#if CONFIG_VP9_ENCODER
  fprintf(stderr, "\nVP9 Specific Options:\n");
  arg_show_usage(stdout, vp9_args);
#endif
  fprintf(stderr, "\nStream timebase (--timebase):\n"
          "  The desired precision of timestamps in the output, expressed\n"
          "  in fractional seconds. Default is 1/1000.\n");
  fprintf(stderr, "\n"
          "Included encoders:\n"
          "\n");

  for (i = 0; i < sizeof(codecs) / sizeof(codecs[0]); i++)
    fprintf(stderr, "    %-6s - %s\n",
            codecs[i].name,
            vpx_codec_iface_name(codecs[i].iface()));

  exit(EXIT_FAILURE);
}


#define HIST_BAR_MAX 40
struct hist_bucket {
  int low, high, count;
};


static int merge_hist_buckets(struct hist_bucket *bucket,
                              int *buckets_,
                              int max_buckets) {
  int small_bucket = 0, merge_bucket = INT_MAX, big_bucket = 0;
  int buckets = *buckets_;
  int i;

  /* Find the extrema for this list of buckets */
  big_bucket = small_bucket = 0;
  for (i = 0; i < buckets; i++) {
    if (bucket[i].count < bucket[small_bucket].count)
      small_bucket = i;
    if (bucket[i].count > bucket[big_bucket].count)
      big_bucket = i;
  }

  /* If we have too many buckets, merge the smallest with an adjacent
   * bucket.
   */
  while (buckets > max_buckets) {
    int last_bucket = buckets - 1;

    /* merge the small bucket with an adjacent one. */
    if (small_bucket == 0)
      merge_bucket = 1;
    else if (small_bucket == last_bucket)
      merge_bucket = last_bucket - 1;
    else if (bucket[small_bucket - 1].count < bucket[small_bucket + 1].count)
      merge_bucket = small_bucket - 1;
    else
      merge_bucket = small_bucket + 1;

    assert(abs(merge_bucket - small_bucket) <= 1);
    assert(small_bucket < buckets);
    assert(big_bucket < buckets);
    assert(merge_bucket < buckets);

    if (merge_bucket < small_bucket) {
      bucket[merge_bucket].high = bucket[small_bucket].high;
      bucket[merge_bucket].count += bucket[small_bucket].count;
    } else {
      bucket[small_bucket].high = bucket[merge_bucket].high;
      bucket[small_bucket].count += bucket[merge_bucket].count;
      merge_bucket = small_bucket;
    }

    assert(bucket[merge_bucket].low != bucket[merge_bucket].high);

    buckets--;

    /* Remove the merge_bucket from the list, and find the new small
     * and big buckets while we're at it
     */
    big_bucket = small_bucket = 0;
    for (i = 0; i < buckets; i++) {
      if (i > merge_bucket)
        bucket[i] = bucket[i + 1];

      if (bucket[i].count < bucket[small_bucket].count)
        small_bucket = i;
      if (bucket[i].count > bucket[big_bucket].count)
        big_bucket = i;
    }

  }

  *buckets_ = buckets;
  return bucket[big_bucket].count;
}


static void show_histogram(const struct hist_bucket *bucket,
                           int                       buckets,
                           int                       total,
                           int                       scale) {
  const char *pat1, *pat2;
  int i;

  switch ((int)(log(bucket[buckets - 1].high) / log(10)) + 1) {
    case 1:
    case 2:
      pat1 = "%4d %2s: ";
      pat2 = "%4d-%2d: ";
      break;
    case 3:
      pat1 = "%5d %3s: ";
      pat2 = "%5d-%3d: ";
      break;
    case 4:
      pat1 = "%6d %4s: ";
      pat2 = "%6d-%4d: ";
      break;
    case 5:
      pat1 = "%7d %5s: ";
      pat2 = "%7d-%5d: ";
      break;
    case 6:
      pat1 = "%8d %6s: ";
      pat2 = "%8d-%6d: ";
      break;
    case 7:
      pat1 = "%9d %7s: ";
      pat2 = "%9d-%7d: ";
      break;
    default:
      pat1 = "%12d %10s: ";
      pat2 = "%12d-%10d: ";
      break;
  }

  for (i = 0; i < buckets; i++) {
    int len;
    int j;
    float pct;

    pct = (float)(100.0 * bucket[i].count / total);
    len = HIST_BAR_MAX * bucket[i].count / scale;
    if (len < 1)
      len = 1;
    assert(len <= HIST_BAR_MAX);

    if (bucket[i].low == bucket[i].high)
      fprintf(stderr, pat1, bucket[i].low, "");
    else
      fprintf(stderr, pat2, bucket[i].low, bucket[i].high);

    for (j = 0; j < HIST_BAR_MAX; j++)
      fprintf(stderr, j < len ? "=" : " ");
    fprintf(stderr, "\t%5d (%6.2f%%)\n", bucket[i].count, pct);
  }
}


static void show_q_histogram(const int counts[64], int max_buckets) {
  struct hist_bucket bucket[64];
  int buckets = 0;
  int total = 0;
  int scale;
  int i;


  for (i = 0; i < 64; i++) {
    if (counts[i]) {
      bucket[buckets].low = bucket[buckets].high = i;
      bucket[buckets].count = counts[i];
      buckets++;
      total += counts[i];
    }
  }

  fprintf(stderr, "\nQuantizer Selection:\n");
  scale = merge_hist_buckets(bucket, &buckets, max_buckets);
  show_histogram(bucket, buckets, total, scale);
}


#define RATE_BINS (100)
struct rate_hist {
  int64_t            *pts;
  int                *sz;
  int                 samples;
  int                 frames;
  struct hist_bucket  bucket[RATE_BINS];
  int                 total;
};


static void init_rate_histogram(struct rate_hist          *hist,
                                const vpx_codec_enc_cfg_t *cfg,
                                const vpx_rational_t      *fps) {
  int i;

  /* Determine the number of samples in the buffer. Use the file's framerate
   * to determine the number of frames in rc_buf_sz milliseconds, with an
   * adjustment (5/4) to account for alt-refs
   */
  hist->samples = cfg->rc_buf_sz * 5 / 4 * fps->num / fps->den / 1000;

  /* prevent division by zero */
  if (hist->samples == 0)
    hist->samples = 1;

  hist->pts = calloc(hist->samples, sizeof(*hist->pts));
  hist->sz = calloc(hist->samples, sizeof(*hist->sz));
  for (i = 0; i < RATE_BINS; i++) {
    hist->bucket[i].low = INT_MAX;
    hist->bucket[i].high = 0;
    hist->bucket[i].count = 0;
  }
}


static void destroy_rate_histogram(struct rate_hist *hist) {
  free(hist->pts);
  free(hist->sz);
}


static void update_rate_histogram(struct rate_hist          *hist,
                                  const vpx_codec_enc_cfg_t *cfg,
                                  const vpx_codec_cx_pkt_t  *pkt) {
  int i, idx;
  int64_t now, then, sum_sz = 0, avg_bitrate;

  now = pkt->data.frame.pts * 1000
        * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;

  idx = hist->frames++ % hist->samples;
  hist->pts[idx] = now;
  hist->sz[idx] = (int)pkt->data.frame.sz;

  if (now < cfg->rc_buf_initial_sz)
    return;

  then = now;

  /* Sum the size over the past rc_buf_sz ms */
  for (i = hist->frames; i > 0 && hist->frames - i < hist->samples; i--) {
    int i_idx = (i - 1) % hist->samples;

    then = hist->pts[i_idx];
    if (now - then > cfg->rc_buf_sz)
      break;
    sum_sz += hist->sz[i_idx];
  }

  if (now == then)
    return;

  avg_bitrate = sum_sz * 8 * 1000 / (now - then);
  idx = (int)(avg_bitrate * (RATE_BINS / 2) / (cfg->rc_target_bitrate * 1000));
  if (idx < 0)
    idx = 0;
  if (idx > RATE_BINS - 1)
    idx = RATE_BINS - 1;
  if (hist->bucket[idx].low > avg_bitrate)
    hist->bucket[idx].low = (int)avg_bitrate;
  if (hist->bucket[idx].high < avg_bitrate)
    hist->bucket[idx].high = (int)avg_bitrate;
  hist->bucket[idx].count++;
  hist->total++;
}


static void show_rate_histogram(struct rate_hist          *hist,
                                const vpx_codec_enc_cfg_t *cfg,
                                int                        max_buckets) {
  int i, scale;
  int buckets = 0;

  for (i = 0; i < RATE_BINS; i++) {
    if (hist->bucket[i].low == INT_MAX)
      continue;
    hist->bucket[buckets++] = hist->bucket[i];
  }

  fprintf(stderr, "\nRate (over %dms window):\n", cfg->rc_buf_sz);
  scale = merge_hist_buckets(hist->bucket, &buckets, max_buckets);
  show_histogram(hist->bucket, buckets, hist->total, scale);
}

#define mmin(a, b)  ((a) < (b) ? (a) : (b))
static void find_mismatch(vpx_image_t *img1, vpx_image_t *img2,
                          int yloc[2], int uloc[2], int vloc[2]) {
  const unsigned int bsize = 64;
  const unsigned int bsize2 = bsize >> 1;
  unsigned int match = 1;
  unsigned int i, j;
  yloc[0] = yloc[1] = yloc[2] = yloc[3] = -1;
  for (i = 0, match = 1; match && i < img1->d_h; i += bsize) {
    for (j = 0; match && j < img1->d_w; j += bsize) {
      int k, l;
      int si = mmin(i + bsize, img1->d_h) - i;
      int sj = mmin(j + bsize, img1->d_w) - j;
      for (k = 0; match && k < si; k++)
        for (l = 0; match && l < sj; l++) {
          if (*(img1->planes[VPX_PLANE_Y] +
                (i + k) * img1->stride[VPX_PLANE_Y] + j + l) !=
              *(img2->planes[VPX_PLANE_Y] +
                (i + k) * img2->stride[VPX_PLANE_Y] + j + l)) {
            yloc[0] = i + k;
            yloc[1] = j + l;
            yloc[2] = *(img1->planes[VPX_PLANE_Y] +
                        (i + k) * img1->stride[VPX_PLANE_Y] + j + l);
            yloc[3] = *(img2->planes[VPX_PLANE_Y] +
                        (i + k) * img2->stride[VPX_PLANE_Y] + j + l);
            match = 0;
            break;
          }
        }
    }
  }
  uloc[0] = uloc[1] = uloc[2] = uloc[3] = -1;
  for (i = 0, match = 1; match && i < (img1->d_h + 1) / 2; i += bsize2) {
    for (j = 0; j < match && (img1->d_w + 1) / 2; j += bsize2) {
      int k, l;
      int si = mmin(i + bsize2, (img1->d_h + 1) / 2) - i;
      int sj = mmin(j + bsize2, (img1->d_w + 1) / 2) - j;
      for (k = 0; match && k < si; k++)
        for (l = 0; match && l < sj; l++) {
          if (*(img1->planes[VPX_PLANE_U] +
                (i + k) * img1->stride[VPX_PLANE_U] + j + l) !=
              *(img2->planes[VPX_PLANE_U] +
                (i + k) * img2->stride[VPX_PLANE_U] + j + l)) {
            uloc[0] = i + k;
            uloc[1] = j + l;
            uloc[2] = *(img1->planes[VPX_PLANE_U] +
                        (i + k) * img1->stride[VPX_PLANE_U] + j + l);
            uloc[3] = *(img2->planes[VPX_PLANE_U] +
                        (i + k) * img2->stride[VPX_PLANE_V] + j + l);
            match = 0;
            break;
          }
        }
    }
  }
  vloc[0] = vloc[1] = vloc[2] = vloc[3] = -1;
  for (i = 0, match = 1; match && i < (img1->d_h + 1) / 2; i += bsize2) {
    for (j = 0; j < match && (img1->d_w + 1) / 2; j += bsize2) {
      int k, l;
      int si = mmin(i + bsize2, (img1->d_h + 1) / 2) - i;
      int sj = mmin(j + bsize2, (img1->d_w + 1) / 2) - j;
      for (k = 0; match && k < si; k++)
        for (l = 0; match && l < sj; l++) {
          if (*(img1->planes[VPX_PLANE_V] +
                (i + k) * img1->stride[VPX_PLANE_V] + j + l) !=
              *(img2->planes[VPX_PLANE_V] +
                (i + k) * img2->stride[VPX_PLANE_V] + j + l)) {
            vloc[0] = i + k;
            vloc[1] = j + l;
            vloc[2] = *(img1->planes[VPX_PLANE_V] +
                        (i + k) * img1->stride[VPX_PLANE_V] + j + l);
            vloc[3] = *(img2->planes[VPX_PLANE_V] +
                        (i + k) * img2->stride[VPX_PLANE_V] + j + l);
            match = 0;
            break;
          }
        }
    }
  }
}

static int compare_img(vpx_image_t *img1, vpx_image_t *img2)
{
  int match = 1;
  unsigned int i;

  match &= (img1->fmt == img2->fmt);
  match &= (img1->w == img2->w);
  match &= (img1->h == img2->h);

  for (i = 0; i < img1->d_h; i++)
    match &= (memcmp(img1->planes[VPX_PLANE_Y]+i*img1->stride[VPX_PLANE_Y],
                     img2->planes[VPX_PLANE_Y]+i*img2->stride[VPX_PLANE_Y],
                     img1->d_w) == 0);

  for (i = 0; i < img1->d_h/2; i++)
    match &= (memcmp(img1->planes[VPX_PLANE_U]+i*img1->stride[VPX_PLANE_U],
                     img2->planes[VPX_PLANE_U]+i*img2->stride[VPX_PLANE_U],
                     (img1->d_w + 1) / 2) == 0);

  for (i = 0; i < img1->d_h/2; i++)
    match &= (memcmp(img1->planes[VPX_PLANE_V]+i*img1->stride[VPX_PLANE_U],
                     img2->planes[VPX_PLANE_V]+i*img2->stride[VPX_PLANE_U],
                     (img1->d_w + 1) / 2) == 0);

  return match;
}


#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#define MAX(x,y) ((x)>(y)?(x):(y))
#if CONFIG_VP8_ENCODER && !CONFIG_VP9_ENCODER
#define ARG_CTRL_CNT_MAX NELEMENTS(vp8_arg_ctrl_map)
#elif !CONFIG_VP8_ENCODER && CONFIG_VP9_ENCODER
#define ARG_CTRL_CNT_MAX NELEMENTS(vp9_arg_ctrl_map)
#else
#define ARG_CTRL_CNT_MAX MAX(NELEMENTS(vp8_arg_ctrl_map), \
                             NELEMENTS(vp9_arg_ctrl_map))
#endif

/* Configuration elements common to all streams */
struct global_config {
  const struct codec_item  *codec;
  int                       passes;
  int                       pass;
  int                       usage;
  int                       deadline;
  int                       use_i420;
  int                       quiet;
  int                       verbose;
  int                       limit;
  int                       skip_frames;
  int                       show_psnr;
  enum TestDecodeFatality   test_decode;
  int                       have_framerate;
  struct vpx_rational       framerate;
  int                       out_part;
  int                       debug;
  int                       show_q_hist_buckets;
  int                       show_rate_hist_buckets;
};


/* Per-stream configuration */
struct stream_config {
  struct vpx_codec_enc_cfg  cfg;
  const char               *out_fn;
  const char               *stats_fn;
  stereo_format_t           stereo_fmt;
  int                       arg_ctrls[ARG_CTRL_CNT_MAX][2];
  int                       arg_ctrl_cnt;
  int                       write_webm;
  int                       have_kf_max_dist;
};


struct stream_state {
  int                       index;
  struct stream_state      *next;
  struct stream_config      config;
  FILE                     *file;
  struct rate_hist          rate_hist;
  EbmlGlobal                ebml;
  uint32_t                  hash;
  uint64_t                  psnr_sse_total;
  uint64_t                  psnr_samples_total;
  double                    psnr_totals[4];
  int                       psnr_count;
  int                       counts[64];
  vpx_codec_ctx_t           encoder;
  unsigned int              frames_out;
  uint64_t                  cx_time;
  size_t                    nbytes;
  stats_io_t                stats;
  struct vpx_image         *img;
  vpx_codec_ctx_t           decoder;
  int                       mismatch_seen;
};


void validate_positive_rational(const char          *msg,
                                struct vpx_rational *rat) {
  if (rat->den < 0) {
    rat->num *= -1;
    rat->den *= -1;
  }

  if (rat->num < 0)
    die("Error: %s must be positive\n", msg);

  if (!rat->den)
    die("Error: %s has zero denominator\n", msg);
}


static void parse_global_config(struct global_config *global, char **argv) {
  char       **argi, **argj;
  struct arg   arg;

  /* Initialize default parameters */
  memset(global, 0, sizeof(*global));
  global->codec = codecs;
  global->passes = 1;
  global->use_i420 = 1;

  for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step) {
    arg.argv_step = 1;

    if (arg_match(&arg, &codecarg, argi)) {
      int j, k = -1;

      for (j = 0; j < sizeof(codecs) / sizeof(codecs[0]); j++)
        if (!strcmp(codecs[j].name, arg.val))
          k = j;

      if (k >= 0)
        global->codec = codecs + k;
      else
        die("Error: Unrecognized argument (%s) to --codec\n",
            arg.val);

    } else if (arg_match(&arg, &passes, argi)) {
      global->passes = arg_parse_uint(&arg);

      if (global->passes < 1 || global->passes > 2)
        die("Error: Invalid number of passes (%d)\n", global->passes);
    } else if (arg_match(&arg, &pass_arg, argi)) {
      global->pass = arg_parse_uint(&arg);

      if (global->pass < 1 || global->pass > 2)
        die("Error: Invalid pass selected (%d)\n",
            global->pass);
    } else if (arg_match(&arg, &usage, argi))
      global->usage = arg_parse_uint(&arg);
    else if (arg_match(&arg, &deadline, argi))
      global->deadline = arg_parse_uint(&arg);
    else if (arg_match(&arg, &best_dl, argi))
      global->deadline = VPX_DL_BEST_QUALITY;
    else if (arg_match(&arg, &good_dl, argi))
      global->deadline = VPX_DL_GOOD_QUALITY;
    else if (arg_match(&arg, &rt_dl, argi))
      global->deadline = VPX_DL_REALTIME;
    else if (arg_match(&arg, &use_yv12, argi))
      global->use_i420 = 0;
    else if (arg_match(&arg, &use_i420, argi))
      global->use_i420 = 1;
    else if (arg_match(&arg, &quietarg, argi))
      global->quiet = 1;
    else if (arg_match(&arg, &verbosearg, argi))
      global->verbose = 1;
    else if (arg_match(&arg, &limit, argi))
      global->limit = arg_parse_uint(&arg);
    else if (arg_match(&arg, &skip, argi))
      global->skip_frames = arg_parse_uint(&arg);
    else if (arg_match(&arg, &psnrarg, argi))
      global->show_psnr = 1;
    else if (arg_match(&arg, &recontest, argi))
      global->test_decode = arg_parse_enum_or_int(&arg);
    else if (arg_match(&arg, &framerate, argi)) {
      global->framerate = arg_parse_rational(&arg);
      validate_positive_rational(arg.name, &global->framerate);
      global->have_framerate = 1;
    } else if (arg_match(&arg, &out_part, argi))
      global->out_part = 1;
    else if (arg_match(&arg, &debugmode, argi))
      global->debug = 1;
    else if (arg_match(&arg, &q_hist_n, argi))
      global->show_q_hist_buckets = arg_parse_uint(&arg);
    else if (arg_match(&arg, &rate_hist_n, argi))
      global->show_rate_hist_buckets = arg_parse_uint(&arg);
    else
      argj++;
  }

  /* Validate global config */

  if (global->pass) {
    /* DWIM: Assume the user meant passes=2 if pass=2 is specified */
    if (global->pass > global->passes) {
      warn("Assuming --pass=%d implies --passes=%d\n",
           global->pass, global->pass);
      global->passes = global->pass;
    }
  }
}


void open_input_file(struct input_state *input) {
  unsigned int fourcc;

  /* Parse certain options from the input file, if possible */
  input->file = strcmp(input->fn, "-") ? fopen(input->fn, "rb")
                : set_binary_mode(stdin);

  if (!input->file)
    fatal("Failed to open input file");

  if (!fseeko(input->file, 0, SEEK_END)) {
    /* Input file is seekable. Figure out how long it is, so we can get
     * progress info.
     */
    input->length = ftello(input->file);
    rewind(input->file);
  }

  /* For RAW input sources, these bytes will applied on the first frame
   *  in read_frame().
   */
  input->detect.buf_read = fread(input->detect.buf, 1, 4, input->file);
  input->detect.position = 0;

  if (input->detect.buf_read == 4
      && file_is_y4m(input->file, &input->y4m, input->detect.buf)) {
    if (y4m_input_open(&input->y4m, input->file, input->detect.buf, 4) >= 0) {
      input->file_type = FILE_TYPE_Y4M;
      input->w = input->y4m.pic_w;
      input->h = input->y4m.pic_h;
      input->framerate.num = input->y4m.fps_n;
      input->framerate.den = input->y4m.fps_d;
      input->use_i420 = 0;
    } else
      fatal("Unsupported Y4M stream.");
  } else if (input->detect.buf_read == 4 && file_is_ivf(input, &fourcc)) {
    input->file_type = FILE_TYPE_IVF;
    switch (fourcc) {
      case 0x32315659:
        input->use_i420 = 0;
        break;
      case 0x30323449:
        input->use_i420 = 1;
        break;
      default:
        fatal("Unsupported fourcc (%08x) in IVF", fourcc);
    }
  } else {
    input->file_type = FILE_TYPE_RAW;
  }
}


static void close_input_file(struct input_state *input) {
  fclose(input->file);
  if (input->file_type == FILE_TYPE_Y4M)
    y4m_input_close(&input->y4m);
}

static struct stream_state *new_stream(struct global_config *global,
                                       struct stream_state  *prev) {
  struct stream_state *stream;

  stream = calloc(1, sizeof(*stream));
  if (!stream)
    fatal("Failed to allocate new stream.");
  if (prev) {
    memcpy(stream, prev, sizeof(*stream));
    stream->index++;
    prev->next = stream;
  } else {
    vpx_codec_err_t  res;

    /* Populate encoder configuration */
    res = vpx_codec_enc_config_default(global->codec->iface(),
                                       &stream->config.cfg,
                                       global->usage);
    if (res)
      fatal("Failed to get config: %s\n", vpx_codec_err_to_string(res));

    /* Change the default timebase to a high enough value so that the
     * encoder will always create strictly increasing timestamps.
     */
    stream->config.cfg.g_timebase.den = 1000;

    /* Never use the library's default resolution, require it be parsed
     * from the file or set on the command line.
     */
    stream->config.cfg.g_w = 0;
    stream->config.cfg.g_h = 0;

    /* Initialize remaining stream parameters */
    stream->config.stereo_fmt = STEREO_FORMAT_MONO;
    stream->config.write_webm = 1;
    stream->ebml.last_pts_ms = -1;

    /* Allows removal of the application version from the EBML tags */
    stream->ebml.debug = global->debug;
  }

  /* Output files must be specified for each stream */
  stream->config.out_fn = NULL;

  stream->next = NULL;
  return stream;
}


static int parse_stream_params(struct global_config *global,
                               struct stream_state  *stream,
                               char **argv) {
  char                   **argi, **argj;
  struct arg               arg;
  static const arg_def_t **ctrl_args = no_args;
  static const int        *ctrl_args_map = NULL;
  struct stream_config    *config = &stream->config;
  int                      eos_mark_found = 0;

  /* Handle codec specific options */
  if (0) {
#if CONFIG_VP8_ENCODER
  } else if (global->codec->iface == vpx_codec_vp8_cx) {
    ctrl_args = vp8_args;
    ctrl_args_map = vp8_arg_ctrl_map;
#endif
#if CONFIG_VP9_ENCODER
  } else if (global->codec->iface == vpx_codec_vp9_cx) {
    ctrl_args = vp9_args;
    ctrl_args_map = vp9_arg_ctrl_map;
#endif
  }

  for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step) {
    arg.argv_step = 1;

    /* Once we've found an end-of-stream marker (--) we want to continue
     * shifting arguments but not consuming them.
     */
    if (eos_mark_found) {
      argj++;
      continue;
    } else if (!strcmp(*argj, "--")) {
      eos_mark_found = 1;
      continue;
    }

    if (0);
    else if (arg_match(&arg, &outputfile, argi))
      config->out_fn = arg.val;
    else if (arg_match(&arg, &fpf_name, argi))
      config->stats_fn = arg.val;
    else if (arg_match(&arg, &use_ivf, argi))
      config->write_webm = 0;
    else if (arg_match(&arg, &threads, argi))
      config->cfg.g_threads = arg_parse_uint(&arg);
    else if (arg_match(&arg, &profile, argi))
      config->cfg.g_profile = arg_parse_uint(&arg);
    else if (arg_match(&arg, &width, argi))
      config->cfg.g_w = arg_parse_uint(&arg);
    else if (arg_match(&arg, &height, argi))
      config->cfg.g_h = arg_parse_uint(&arg);
    else if (arg_match(&arg, &stereo_mode, argi))
      config->stereo_fmt = arg_parse_enum_or_int(&arg);
    else if (arg_match(&arg, &timebase, argi)) {
      config->cfg.g_timebase = arg_parse_rational(&arg);
      validate_positive_rational(arg.name, &config->cfg.g_timebase);
    } else if (arg_match(&arg, &error_resilient, argi))
      config->cfg.g_error_resilient = arg_parse_uint(&arg);
    else if (arg_match(&arg, &lag_in_frames, argi))
      config->cfg.g_lag_in_frames = arg_parse_uint(&arg);
    else if (arg_match(&arg, &dropframe_thresh, argi))
      config->cfg.rc_dropframe_thresh = arg_parse_uint(&arg);
    else if (arg_match(&arg, &resize_allowed, argi))
      config->cfg.rc_resize_allowed = arg_parse_uint(&arg);
    else if (arg_match(&arg, &resize_up_thresh, argi))
      config->cfg.rc_resize_up_thresh = arg_parse_uint(&arg);
    else if (arg_match(&arg, &resize_down_thresh, argi))
      config->cfg.rc_resize_down_thresh = arg_parse_uint(&arg);
    else if (arg_match(&arg, &end_usage, argi))
      config->cfg.rc_end_usage = arg_parse_enum_or_int(&arg);
    else if (arg_match(&arg, &target_bitrate, argi))
      config->cfg.rc_target_bitrate = arg_parse_uint(&arg);
    else if (arg_match(&arg, &min_quantizer, argi))
      config->cfg.rc_min_quantizer = arg_parse_uint(&arg);
    else if (arg_match(&arg, &max_quantizer, argi))
      config->cfg.rc_max_quantizer = arg_parse_uint(&arg);
    else if (arg_match(&arg, &undershoot_pct, argi))
      config->cfg.rc_undershoot_pct = arg_parse_uint(&arg);
    else if (arg_match(&arg, &overshoot_pct, argi))
      config->cfg.rc_overshoot_pct = arg_parse_uint(&arg);
    else if (arg_match(&arg, &buf_sz, argi))
      config->cfg.rc_buf_sz = arg_parse_uint(&arg);
    else if (arg_match(&arg, &buf_initial_sz, argi))
      config->cfg.rc_buf_initial_sz = arg_parse_uint(&arg);
    else if (arg_match(&arg, &buf_optimal_sz, argi))
      config->cfg.rc_buf_optimal_sz = arg_parse_uint(&arg);
    else if (arg_match(&arg, &bias_pct, argi)) {
      config->cfg.rc_2pass_vbr_bias_pct = arg_parse_uint(&arg);

      if (global->passes < 2)
        warn("option %s ignored in one-pass mode.\n", arg.name);
    } else if (arg_match(&arg, &minsection_pct, argi)) {
      config->cfg.rc_2pass_vbr_minsection_pct = arg_parse_uint(&arg);

      if (global->passes < 2)
        warn("option %s ignored in one-pass mode.\n", arg.name);
    } else if (arg_match(&arg, &maxsection_pct, argi)) {
      config->cfg.rc_2pass_vbr_maxsection_pct = arg_parse_uint(&arg);

      if (global->passes < 2)
        warn("option %s ignored in one-pass mode.\n", arg.name);
    } else if (arg_match(&arg, &kf_min_dist, argi))
      config->cfg.kf_min_dist = arg_parse_uint(&arg);
    else if (arg_match(&arg, &kf_max_dist, argi)) {
      config->cfg.kf_max_dist = arg_parse_uint(&arg);
      config->have_kf_max_dist = 1;
    } else if (arg_match(&arg, &kf_disabled, argi))
      config->cfg.kf_mode = VPX_KF_DISABLED;
    else {
      int i, match = 0;

      for (i = 0; ctrl_args[i]; i++) {
        if (arg_match(&arg, ctrl_args[i], argi)) {
          int j;
          match = 1;

          /* Point either to the next free element or the first
          * instance of this control.
          */
          for (j = 0; j < config->arg_ctrl_cnt; j++)
            if (config->arg_ctrls[j][0] == ctrl_args_map[i])
              break;

          /* Update/insert */
          assert(j < ARG_CTRL_CNT_MAX);
          if (j < ARG_CTRL_CNT_MAX) {
            config->arg_ctrls[j][0] = ctrl_args_map[i];
            config->arg_ctrls[j][1] = arg_parse_enum_or_int(&arg);
            if (j == config->arg_ctrl_cnt)
              config->arg_ctrl_cnt++;
          }

        }
      }

      if (!match)
        argj++;
    }
  }

  return eos_mark_found;
}


#define FOREACH_STREAM(func)\
  do\
  {\
    struct stream_state  *stream;\
    \
    for(stream = streams; stream; stream = stream->next)\
      func;\
  }while(0)


static void validate_stream_config(struct stream_state *stream) {
  struct stream_state *streami;

  if (!stream->config.cfg.g_w || !stream->config.cfg.g_h)
    fatal("Stream %d: Specify stream dimensions with --width (-w) "
          " and --height (-h)", stream->index);

  for (streami = stream; streami; streami = streami->next) {
    /* All streams require output files */
    if (!streami->config.out_fn)
      fatal("Stream %d: Output file is required (specify with -o)",
            streami->index);

    /* Check for two streams outputting to the same file */
    if (streami != stream) {
      const char *a = stream->config.out_fn;
      const char *b = streami->config.out_fn;
      if (!strcmp(a, b) && strcmp(a, "/dev/null") && strcmp(a, ":nul"))
        fatal("Stream %d: duplicate output file (from stream %d)",
              streami->index, stream->index);
    }

    /* Check for two streams sharing a stats file. */
    if (streami != stream) {
      const char *a = stream->config.stats_fn;
      const char *b = streami->config.stats_fn;
      if (a && b && !strcmp(a, b))
        fatal("Stream %d: duplicate stats file (from stream %d)",
              streami->index, stream->index);
    }
  }
}


static void set_stream_dimensions(struct stream_state *stream,
                                  unsigned int w,
                                  unsigned int h) {
  if (!stream->config.cfg.g_w) {
    if (!stream->config.cfg.g_h)
      stream->config.cfg.g_w = w;
    else
      stream->config.cfg.g_w = w * stream->config.cfg.g_h / h;
  }
  if (!stream->config.cfg.g_h) {
    stream->config.cfg.g_h = h * stream->config.cfg.g_w / w;
  }
}


static void set_default_kf_interval(struct stream_state  *stream,
                                    struct global_config *global) {
  /* Use a max keyframe interval of 5 seconds, if none was
   * specified on the command line.
   */
  if (!stream->config.have_kf_max_dist) {
    double framerate = (double)global->framerate.num / global->framerate.den;
    if (framerate > 0.0)
      stream->config.cfg.kf_max_dist = (unsigned int)(5.0 * framerate);
  }
}


static void show_stream_config(struct stream_state  *stream,
                               struct global_config *global,
                               struct input_state   *input) {

#define SHOW(field) \
  fprintf(stderr, "    %-28s = %d\n", #field, stream->config.cfg.field)

  if (stream->index == 0) {
    fprintf(stderr, "Codec: %s\n",
            vpx_codec_iface_name(global->codec->iface()));
    fprintf(stderr, "Source file: %s Format: %s\n", input->fn,
            input->use_i420 ? "I420" : "YV12");
  }
  if (stream->next || stream->index)
    fprintf(stderr, "\nStream Index: %d\n", stream->index);
  fprintf(stderr, "Destination file: %s\n", stream->config.out_fn);
  fprintf(stderr, "Encoder parameters:\n");

  SHOW(g_usage);
  SHOW(g_threads);
  SHOW(g_profile);
  SHOW(g_w);
  SHOW(g_h);
  SHOW(g_timebase.num);
  SHOW(g_timebase.den);
  SHOW(g_error_resilient);
  SHOW(g_pass);
  SHOW(g_lag_in_frames);
  SHOW(rc_dropframe_thresh);
  SHOW(rc_resize_allowed);
  SHOW(rc_resize_up_thresh);
  SHOW(rc_resize_down_thresh);
  SHOW(rc_end_usage);
  SHOW(rc_target_bitrate);
  SHOW(rc_min_quantizer);
  SHOW(rc_max_quantizer);
  SHOW(rc_undershoot_pct);
  SHOW(rc_overshoot_pct);
  SHOW(rc_buf_sz);
  SHOW(rc_buf_initial_sz);
  SHOW(rc_buf_optimal_sz);
  SHOW(rc_2pass_vbr_bias_pct);
  SHOW(rc_2pass_vbr_minsection_pct);
  SHOW(rc_2pass_vbr_maxsection_pct);
  SHOW(kf_mode);
  SHOW(kf_min_dist);
  SHOW(kf_max_dist);
}


static void open_output_file(struct stream_state *stream,
                             struct global_config *global) {
  const char *fn = stream->config.out_fn;

  stream->file = strcmp(fn, "-") ? fopen(fn, "wb") : set_binary_mode(stdout);

  if (!stream->file)
    fatal("Failed to open output file");

  if (stream->config.write_webm && fseek(stream->file, 0, SEEK_CUR))
    fatal("WebM output to pipes not supported.");

  if (stream->config.write_webm) {
    stream->ebml.stream = stream->file;
    write_webm_file_header(&stream->ebml, &stream->config.cfg,
                           &global->framerate,
                           stream->config.stereo_fmt,
                           global->codec->fourcc);
  } else
    write_ivf_file_header(stream->file, &stream->config.cfg,
                          global->codec->fourcc, 0);
}


static void close_output_file(struct stream_state *stream,
                              unsigned int         fourcc) {
  if (stream->config.write_webm) {
    write_webm_file_footer(&stream->ebml, stream->hash);
    free(stream->ebml.cue_list);
    stream->ebml.cue_list = NULL;
  } else {
    if (!fseek(stream->file, 0, SEEK_SET))
      write_ivf_file_header(stream->file, &stream->config.cfg,
                            fourcc,
                            stream->frames_out);
  }

  fclose(stream->file);
}


static void setup_pass(struct stream_state  *stream,
                       struct global_config *global,
                       int                   pass) {
  if (stream->config.stats_fn) {
    if (!stats_open_file(&stream->stats, stream->config.stats_fn,
                         pass))
      fatal("Failed to open statistics store");
  } else {
    if (!stats_open_mem(&stream->stats, pass))
      fatal("Failed to open statistics store");
  }

  stream->config.cfg.g_pass = global->passes == 2
                              ? pass ? VPX_RC_LAST_PASS : VPX_RC_FIRST_PASS
                            : VPX_RC_ONE_PASS;
  if (pass)
    stream->config.cfg.rc_twopass_stats_in = stats_get(&stream->stats);

  stream->cx_time = 0;
  stream->nbytes = 0;
  stream->frames_out = 0;
}


static void initialize_encoder(struct stream_state  *stream,
                               struct global_config *global) {
  int i;
  int flags = 0;

  flags |= global->show_psnr ? VPX_CODEC_USE_PSNR : 0;
  flags |= global->out_part ? VPX_CODEC_USE_OUTPUT_PARTITION : 0;

  /* Construct Encoder Context */
  vpx_codec_enc_init(&stream->encoder, global->codec->iface(),
                     &stream->config.cfg, flags);
  ctx_exit_on_error(&stream->encoder, "Failed to initialize encoder");

  /* Note that we bypass the vpx_codec_control wrapper macro because
   * we're being clever to store the control IDs in an array. Real
   * applications will want to make use of the enumerations directly
   */
  for (i = 0; i < stream->config.arg_ctrl_cnt; i++) {
    int ctrl = stream->config.arg_ctrls[i][0];
    int value = stream->config.arg_ctrls[i][1];
    if (vpx_codec_control_(&stream->encoder, ctrl, value))
      fprintf(stderr, "Error: Tried to set control %d = %d\n",
              ctrl, value);

    ctx_exit_on_error(&stream->encoder, "Failed to control codec");
  }

#if CONFIG_DECODERS
  if (global->test_decode != TEST_DECODE_OFF) {
    vpx_codec_dec_init(&stream->decoder, global->codec->dx_iface(), NULL, 0);
  }
#endif
}


static void encode_frame(struct stream_state  *stream,
                         struct global_config *global,
                         struct vpx_image     *img,
                         unsigned int          frames_in) {
  vpx_codec_pts_t frame_start, next_frame_start;
  struct vpx_codec_enc_cfg *cfg = &stream->config.cfg;
  struct vpx_usec_timer timer;

  frame_start = (cfg->g_timebase.den * (int64_t)(frames_in - 1)
                 * global->framerate.den)
                / cfg->g_timebase.num / global->framerate.num;
  next_frame_start = (cfg->g_timebase.den * (int64_t)(frames_in)
                      * global->framerate.den)
                     / cfg->g_timebase.num / global->framerate.num;

  /* Scale if necessary */
  if (img && (img->d_w != cfg->g_w || img->d_h != cfg->g_h)) {
    if (!stream->img)
      stream->img = vpx_img_alloc(NULL, VPX_IMG_FMT_I420,
                                  cfg->g_w, cfg->g_h, 16);
    I420Scale(img->planes[VPX_PLANE_Y], img->stride[VPX_PLANE_Y],
              img->planes[VPX_PLANE_U], img->stride[VPX_PLANE_U],
              img->planes[VPX_PLANE_V], img->stride[VPX_PLANE_V],
              img->d_w, img->d_h,
              stream->img->planes[VPX_PLANE_Y],
              stream->img->stride[VPX_PLANE_Y],
              stream->img->planes[VPX_PLANE_U],
              stream->img->stride[VPX_PLANE_U],
              stream->img->planes[VPX_PLANE_V],
              stream->img->stride[VPX_PLANE_V],
              stream->img->d_w, stream->img->d_h,
              kFilterBox);

    img = stream->img;
  }

  vpx_usec_timer_start(&timer);
  vpx_codec_encode(&stream->encoder, img, frame_start,
                   (unsigned long)(next_frame_start - frame_start),
                   0, global->deadline);
  vpx_usec_timer_mark(&timer);
  stream->cx_time += vpx_usec_timer_elapsed(&timer);
  ctx_exit_on_error(&stream->encoder, "Stream %d: Failed to encode frame",
                    stream->index);
}


static void update_quantizer_histogram(struct stream_state *stream) {
  if (stream->config.cfg.g_pass != VPX_RC_FIRST_PASS) {
    int q;

    vpx_codec_control(&stream->encoder, VP8E_GET_LAST_QUANTIZER_64, &q);
    ctx_exit_on_error(&stream->encoder, "Failed to read quantizer");
    stream->counts[q]++;
  }
}


static void get_cx_data(struct stream_state  *stream,
                        struct global_config *global,
                        int                  *got_data) {
  const vpx_codec_cx_pkt_t *pkt;
  const struct vpx_codec_enc_cfg *cfg = &stream->config.cfg;
  vpx_codec_iter_t iter = NULL;

  *got_data = 0;
  while ((pkt = vpx_codec_get_cx_data(&stream->encoder, &iter))) {
    static size_t fsize = 0;
    static off_t ivf_header_pos = 0;

    switch (pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT:
        if (!(pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT)) {
          stream->frames_out++;
        }
        if (!global->quiet)
          fprintf(stderr, " %6luF", (unsigned long)pkt->data.frame.sz);

        update_rate_histogram(&stream->rate_hist, cfg, pkt);
        if (stream->config.write_webm) {
          /* Update the hash */
          if (!stream->ebml.debug)
            stream->hash = murmur(pkt->data.frame.buf,
                                  (int)pkt->data.frame.sz,
                                  stream->hash);

          write_webm_block(&stream->ebml, cfg, pkt);
        } else {
          if (pkt->data.frame.partition_id <= 0) {
            ivf_header_pos = ftello(stream->file);
            fsize = pkt->data.frame.sz;

            write_ivf_frame_header(stream->file, pkt);
          } else {
            fsize += pkt->data.frame.sz;

            if (!(pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT)) {
              off_t currpos = ftello(stream->file);
              fseeko(stream->file, ivf_header_pos, SEEK_SET);
              write_ivf_frame_size(stream->file, fsize);
              fseeko(stream->file, currpos, SEEK_SET);
            }
          }

          (void) fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz,
                        stream->file);
        }
        stream->nbytes += pkt->data.raw.sz;

        *got_data = 1;
#if CONFIG_DECODERS
        if (global->test_decode != TEST_DECODE_OFF && !stream->mismatch_seen) {
          vpx_codec_decode(&stream->decoder, pkt->data.frame.buf,
                           pkt->data.frame.sz, NULL, 0);
          if (stream->decoder.err) {
            warn_or_exit_on_error(&stream->decoder,
                                  global->test_decode == TEST_DECODE_FATAL,
                                  "Failed to decode frame %d in stream %d",
                                  stream->frames_out + 1, stream->index);
            stream->mismatch_seen = stream->frames_out + 1;
          }
        }
#endif
        break;
      case VPX_CODEC_STATS_PKT:
        stream->frames_out++;
        stats_write(&stream->stats,
                    pkt->data.twopass_stats.buf,
                    pkt->data.twopass_stats.sz);
        stream->nbytes += pkt->data.raw.sz;
        break;
      case VPX_CODEC_PSNR_PKT:

        if (global->show_psnr) {
          int i;

          stream->psnr_sse_total += pkt->data.psnr.sse[0];
          stream->psnr_samples_total += pkt->data.psnr.samples[0];
          for (i = 0; i < 4; i++) {
            if (!global->quiet)
              fprintf(stderr, "%.3f ", pkt->data.psnr.psnr[i]);
            stream->psnr_totals[i] += pkt->data.psnr.psnr[i];
          }
          stream->psnr_count++;
        }

        break;
      default:
        break;
    }
  }
}


static void show_psnr(struct stream_state  *stream) {
  int i;
  double ovpsnr;

  if (!stream->psnr_count)
    return;

  fprintf(stderr, "Stream %d PSNR (Overall/Avg/Y/U/V)", stream->index);
  ovpsnr = vp8_mse2psnr((double)stream->psnr_samples_total, 255.0,
                        (double)stream->psnr_sse_total);
  fprintf(stderr, " %.3f", ovpsnr);

  for (i = 0; i < 4; i++) {
    fprintf(stderr, " %.3f", stream->psnr_totals[i] / stream->psnr_count);
  }
  fprintf(stderr, "\n");
}


static float usec_to_fps(uint64_t usec, unsigned int frames) {
  return (float)(usec > 0 ? frames * 1000000.0 / (float)usec : 0);
}


static void test_decode(struct stream_state  *stream,
                        enum TestDecodeFatality fatal,
                        const struct codec_item *codec) {
  vpx_image_t enc_img, dec_img;

  if (stream->mismatch_seen)
    return;

  /* Get the internal reference frame */
  if (codec->fourcc == VP8_FOURCC) {
    struct vpx_ref_frame ref_enc, ref_dec;
    int width, height;

    width = (stream->config.cfg.g_w + 15) & ~15;
    height = (stream->config.cfg.g_h + 15) & ~15;
    vpx_img_alloc(&ref_enc.img, VPX_IMG_FMT_I420, width, height, 1);
    enc_img = ref_enc.img;
    vpx_img_alloc(&ref_dec.img, VPX_IMG_FMT_I420, width, height, 1);
    dec_img = ref_dec.img;

    ref_enc.frame_type = VP8_LAST_FRAME;
    ref_dec.frame_type = VP8_LAST_FRAME;
    vpx_codec_control(&stream->encoder, VP8_COPY_REFERENCE, &ref_enc);
    vpx_codec_control(&stream->decoder, VP8_COPY_REFERENCE, &ref_dec);
  } else {
    struct vp9_ref_frame ref;

    ref.idx = 0;
    vpx_codec_control(&stream->encoder, VP9_GET_REFERENCE, &ref);
    enc_img = ref.img;
    vpx_codec_control(&stream->decoder, VP9_GET_REFERENCE, &ref);
    dec_img = ref.img;
  }
  ctx_exit_on_error(&stream->encoder, "Failed to get encoder reference frame");
  ctx_exit_on_error(&stream->decoder, "Failed to get decoder reference frame");

  if (!compare_img(&enc_img, &dec_img)) {
    int y[4], u[4], v[4];
    find_mismatch(&enc_img, &dec_img, y, u, v);
    stream->decoder.err = 1;
    warn_or_exit_on_error(&stream->decoder, fatal == TEST_DECODE_FATAL,
                          "Stream %d: Encode/decode mismatch on frame %d at"
                          " Y[%d, %d] {%d/%d},"
                          " U[%d, %d] {%d/%d},"
                          " V[%d, %d] {%d/%d}",
                          stream->index, stream->frames_out,
                          y[0], y[1], y[2], y[3],
                          u[0], u[1], u[2], u[3],
                          v[0], v[1], v[2], v[3]);
    stream->mismatch_seen = stream->frames_out;
  }

  vpx_img_free(&enc_img);
  vpx_img_free(&dec_img);
}


static void print_time(const char *label, int64_t etl) {
  int hours, mins, secs;

  if (etl >= 0) {
    hours = etl / 3600;
    etl -= hours * 3600;
    mins = etl / 60;
    etl -= mins * 60;
    secs = etl;

    fprintf(stderr, "[%3s %2d:%02d:%02d] ",
            label, hours, mins, secs);
  } else {
    fprintf(stderr, "[%3s  unknown] ", label);
  }
}

int main(int argc, const char **argv_) {
  int                    pass;
  vpx_image_t            raw;
  int                    frame_avail, got_data;

  struct input_state       input = {0};
  struct global_config     global;
  struct stream_state     *streams = NULL;
  char                   **argv, **argi;
  uint64_t                 cx_time = 0;
  int                      stream_cnt = 0;
  int                      res = 0;

  exec_name = argv_[0];

  if (argc < 3)
    usage_exit();

  /* Setup default input stream settings */
  input.framerate.num = 30;
  input.framerate.den = 1;
  input.use_i420 = 1;

  /* First parse the global configuration values, because we want to apply
   * other parameters on top of the default configuration provided by the
   * codec.
   */
  argv = argv_dup(argc - 1, argv_ + 1);
  parse_global_config(&global, argv);

  {
    /* Now parse each stream's parameters. Using a local scope here
     * due to the use of 'stream' as loop variable in FOREACH_STREAM
     * loops
     */
    struct stream_state *stream = NULL;

    do {
      stream = new_stream(&global, stream);
      stream_cnt++;
      if (!streams)
        streams = stream;
    } while (parse_stream_params(&global, stream, argv));
  }

  /* Check for unrecognized options */
  for (argi = argv; *argi; argi++)
    if (argi[0][0] == '-' && argi[0][1])
      die("Error: Unrecognized option %s\n", *argi);

  /* Handle non-option arguments */
  input.fn = argv[0];

  if (!input.fn)
    usage_exit();

  for (pass = global.pass ? global.pass - 1 : 0; pass < global.passes; pass++) {
    int frames_in = 0, seen_frames = 0;
    int64_t estimated_time_left = -1;
    int64_t average_rate = -1;
    off_t lagged_count = 0;

    open_input_file(&input);

    /* If the input file doesn't specify its w/h (raw files), try to get
     * the data from the first stream's configuration.
     */
    if (!input.w || !input.h)
      FOREACH_STREAM( {
      if (stream->config.cfg.g_w && stream->config.cfg.g_h) {
        input.w = stream->config.cfg.g_w;
        input.h = stream->config.cfg.g_h;
        break;
      }
    });

    /* Update stream configurations from the input file's parameters */
    if (!input.w || !input.h)
      fatal("Specify stream dimensions with --width (-w) "
            " and --height (-h)");
    FOREACH_STREAM(set_stream_dimensions(stream, input.w, input.h));
    FOREACH_STREAM(validate_stream_config(stream));

    /* Ensure that --passes and --pass are consistent. If --pass is set and
     * --passes=2, ensure --fpf was set.
     */
    if (global.pass && global.passes == 2)
      FOREACH_STREAM( {
      if (!stream->config.stats_fn)
        die("Stream %d: Must specify --fpf when --pass=%d"
        " and --passes=2\n", stream->index, global.pass);
    });

    /* Use the frame rate from the file only if none was specified
     * on the command-line.
     */
    if (!global.have_framerate)
      global.framerate = input.framerate;

    FOREACH_STREAM(set_default_kf_interval(stream, &global));

    /* Show configuration */
    if (global.verbose && pass == 0)
      FOREACH_STREAM(show_stream_config(stream, &global, &input));

    if (pass == (global.pass ? global.pass - 1 : 0)) {
      if (input.file_type == FILE_TYPE_Y4M)
        /*The Y4M reader does its own allocation.
          Just initialize this here to avoid problems if we never read any
           frames.*/
        memset(&raw, 0, sizeof(raw));
      else
        vpx_img_alloc(&raw,
                      input.use_i420 ? VPX_IMG_FMT_I420
                      : VPX_IMG_FMT_YV12,
                      input.w, input.h, 32);

      FOREACH_STREAM(init_rate_histogram(&stream->rate_hist,
                                         &stream->config.cfg,
                                         &global.framerate));
    }

    FOREACH_STREAM(open_output_file(stream, &global));
    FOREACH_STREAM(setup_pass(stream, &global, pass));
    FOREACH_STREAM(initialize_encoder(stream, &global));

    frame_avail = 1;
    got_data = 0;

    while (frame_avail || got_data) {
      struct vpx_usec_timer timer;

      if (!global.limit || frames_in < global.limit) {
        frame_avail = read_frame(&input, &raw);

        if (frame_avail)
          frames_in++;
        seen_frames = frames_in > global.skip_frames ?
                          frames_in - global.skip_frames : 0;

        if (!global.quiet) {
          float fps = usec_to_fps(cx_time, seen_frames);
          fprintf(stderr, "\rPass %d/%d ", pass + 1, global.passes);

          if (stream_cnt == 1)
            fprintf(stderr,
                    "frame %4d/%-4d %7"PRId64"B ",
                    frames_in, streams->frames_out, (int64_t)streams->nbytes);
          else
            fprintf(stderr, "frame %4d ", frames_in);

          fprintf(stderr, "%7"PRId64" %s %.2f %s ",
                  cx_time > 9999999 ? cx_time / 1000 : cx_time,
                  cx_time > 9999999 ? "ms" : "us",
                  fps >= 1.0 ? fps : 1000.0 / fps,
                  fps >= 1.0 ? "fps" : "ms/f");
          print_time("ETA", estimated_time_left);
          fprintf(stderr, "\033[K");
        }

      } else
        frame_avail = 0;

      if (frames_in > global.skip_frames) {
        vpx_usec_timer_start(&timer);
        FOREACH_STREAM(encode_frame(stream, &global,
                                    frame_avail ? &raw : NULL,
                                    frames_in));
        vpx_usec_timer_mark(&timer);
        cx_time += vpx_usec_timer_elapsed(&timer);

        FOREACH_STREAM(update_quantizer_histogram(stream));

        got_data = 0;
        FOREACH_STREAM(get_cx_data(stream, &global, &got_data));

        if (!got_data && input.length && !streams->frames_out) {
          lagged_count = global.limit ? seen_frames : ftello(input.file);
        } else if (input.length) {
          int64_t remaining;
          int64_t rate;

          if (global.limit) {
            int frame_in_lagged = (seen_frames - lagged_count) * 1000;

            rate = cx_time ? frame_in_lagged * (int64_t)1000000 / cx_time : 0;
            remaining = 1000 * (global.limit - global.skip_frames
                                - seen_frames + lagged_count);
          } else {
            off_t input_pos = ftello(input.file);
            off_t input_pos_lagged = input_pos - lagged_count;
            int64_t limit = input.length;

            rate = cx_time ? input_pos_lagged * (int64_t)1000000 / cx_time : 0;
            remaining = limit - input_pos + lagged_count;
          }

          average_rate = (average_rate <= 0)
              ? rate
              : (average_rate * 7 + rate) / 8;
          estimated_time_left = average_rate ? remaining / average_rate : -1;
        }

        if (got_data && global.test_decode != TEST_DECODE_OFF)
          FOREACH_STREAM(test_decode(stream, global.test_decode, global.codec));
      }

      fflush(stdout);
    }

    if (stream_cnt > 1)
      fprintf(stderr, "\n");

    if (!global.quiet)
      FOREACH_STREAM(fprintf(
                       stderr,
                       "\rPass %d/%d frame %4d/%-4d %7"PRId64"B %7lub/f %7"PRId64"b/s"
                       " %7"PRId64" %s (%.2f fps)\033[K\n", pass + 1,
                       global.passes, frames_in, stream->frames_out, (int64_t)stream->nbytes,
                       seen_frames ? (unsigned long)(stream->nbytes * 8 / seen_frames) : 0,
                       seen_frames ? (int64_t)stream->nbytes * 8
                       * (int64_t)global.framerate.num / global.framerate.den
                       / seen_frames
                       : 0,
                       stream->cx_time > 9999999 ? stream->cx_time / 1000 : stream->cx_time,
                       stream->cx_time > 9999999 ? "ms" : "us",
                       usec_to_fps(stream->cx_time, seen_frames));
                    );

    if (global.show_psnr)
      FOREACH_STREAM(show_psnr(stream));

    FOREACH_STREAM(vpx_codec_destroy(&stream->encoder));

    if (global.test_decode != TEST_DECODE_OFF) {
      FOREACH_STREAM(vpx_codec_destroy(&stream->decoder));
    }

    close_input_file(&input);

    if (global.test_decode == TEST_DECODE_FATAL) {
      FOREACH_STREAM(res |= stream->mismatch_seen);
    }
    FOREACH_STREAM(close_output_file(stream, global.codec->fourcc));

    FOREACH_STREAM(stats_close(&stream->stats, global.passes - 1));

    if (global.pass)
      break;
  }

  if (global.show_q_hist_buckets)
    FOREACH_STREAM(show_q_histogram(stream->counts,
                                    global.show_q_hist_buckets));

  if (global.show_rate_hist_buckets)
    FOREACH_STREAM(show_rate_histogram(&stream->rate_hist,
                                       &stream->config.cfg,
                                       global.show_rate_hist_buckets));
  FOREACH_STREAM(destroy_rate_histogram(&stream->rate_hist));

#if CONFIG_INTERNAL_STATS
  /* TODO(jkoleszar): This doesn't belong in this executable. Do it for now,
   * to match some existing utilities.
   */
  FOREACH_STREAM({
    FILE *f = fopen("opsnr.stt", "a");
    if (stream->mismatch_seen) {
      fprintf(f, "First mismatch occurred in frame %d\n",
              stream->mismatch_seen);
    } else {
      fprintf(f, "No mismatch detected in recon buffers\n");
    }
    fclose(f);
  });
#endif

  vpx_img_free(&raw);
  free(argv);
  free(streams);
  return res ? EXIT_FAILURE : EXIT_SUCCESS;
}
