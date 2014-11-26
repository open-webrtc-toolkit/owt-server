/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/* This is a simple program that reads ivf files and decodes them
 * using the new interface. Decoded frames are output as YV12 raw.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx_config.h"
#include "vpx/vpx_decoder.h"
#include "vpx_ports/vpx_timer.h"
#if CONFIG_VP8_DECODER || CONFIG_VP9_DECODER
#include "vpx/vp8dx.h"
#endif
#if CONFIG_MD5
#include "md5_utils.h"
#endif
#include "tools_common.h"
#include "nestegg/include/nestegg/nestegg.h"
#include "third_party/libyuv/include/libyuv/scale.h"

#if CONFIG_OS_SUPPORT
#if defined(_MSC_VER)
#include <io.h>
#define snprintf _snprintf
#define isatty   _isatty
#define fileno   _fileno
#else
#include <unistd.h>
#endif
#endif

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

static const char *exec_name;

#define VP8_FOURCC (0x00385056)
#define VP9_FOURCC (0x00395056)
static const struct {
  char const *name;
  const vpx_codec_iface_t *(*iface)(void);
  unsigned int             fourcc;
  unsigned int             fourcc_mask;
} ifaces[] = {
#if CONFIG_VP8_DECODER
  {"vp8",  vpx_codec_vp8_dx,   VP8_FOURCC, 0x00FFFFFF},
#endif
#if CONFIG_VP9_DECODER
  {"vp9",  vpx_codec_vp9_dx,   VP9_FOURCC, 0x00FFFFFF},
#endif
};

#include "args.h"
static const arg_def_t codecarg = ARG_DEF(NULL, "codec", 1,
                                          "Codec to use");
static const arg_def_t use_yv12 = ARG_DEF(NULL, "yv12", 0,
                                          "Output raw YV12 frames");
static const arg_def_t use_i420 = ARG_DEF(NULL, "i420", 0,
                                          "Output raw I420 frames");
static const arg_def_t flipuvarg = ARG_DEF(NULL, "flipuv", 0,
                                           "Flip the chroma planes in the output");
static const arg_def_t noblitarg = ARG_DEF(NULL, "noblit", 0,
                                           "Don't process the decoded frames");
static const arg_def_t progressarg = ARG_DEF(NULL, "progress", 0,
                                             "Show progress after each frame decodes");
static const arg_def_t limitarg = ARG_DEF(NULL, "limit", 1,
                                          "Stop decoding after n frames");
static const arg_def_t skiparg = ARG_DEF(NULL, "skip", 1,
                                         "Skip the first n input frames");
static const arg_def_t postprocarg = ARG_DEF(NULL, "postproc", 0,
                                             "Postprocess decoded frames");
static const arg_def_t summaryarg = ARG_DEF(NULL, "summary", 0,
                                            "Show timing summary");
static const arg_def_t outputfile = ARG_DEF("o", "output", 1,
                                            "Output file name pattern (see below)");
static const arg_def_t threadsarg = ARG_DEF("t", "threads", 1,
                                            "Max threads to use");
static const arg_def_t verbosearg = ARG_DEF("v", "verbose", 0,
                                            "Show version string");
static const arg_def_t error_concealment = ARG_DEF(NULL, "error-concealment", 0,
                                                   "Enable decoder error-concealment");
static const arg_def_t scalearg = ARG_DEF("S", "scale", 0,
                                            "Scale output frames uniformly");


#if CONFIG_MD5
static const arg_def_t md5arg = ARG_DEF(NULL, "md5", 0,
                                        "Compute the MD5 sum of the decoded frame");
#endif
static const arg_def_t *all_args[] = {
  &codecarg, &use_yv12, &use_i420, &flipuvarg, &noblitarg,
  &progressarg, &limitarg, &skiparg, &postprocarg, &summaryarg, &outputfile,
  &threadsarg, &verbosearg, &scalearg,
#if CONFIG_MD5
  &md5arg,
#endif
  &error_concealment,
  NULL
};

#if CONFIG_VP8_DECODER
static const arg_def_t addnoise_level = ARG_DEF(NULL, "noise-level", 1,
                                                "Enable VP8 postproc add noise");
static const arg_def_t deblock = ARG_DEF(NULL, "deblock", 0,
                                         "Enable VP8 deblocking");
static const arg_def_t demacroblock_level = ARG_DEF(NULL, "demacroblock-level", 1,
                                                    "Enable VP8 demacroblocking, w/ level");
static const arg_def_t pp_debug_info = ARG_DEF(NULL, "pp-debug-info", 1,
                                               "Enable VP8 visible debug info");
static const arg_def_t pp_disp_ref_frame = ARG_DEF(NULL, "pp-dbg-ref-frame", 1,
                                                   "Display only selected reference frame per macro block");
static const arg_def_t pp_disp_mb_modes = ARG_DEF(NULL, "pp-dbg-mb-modes", 1,
                                                  "Display only selected macro block modes");
static const arg_def_t pp_disp_b_modes = ARG_DEF(NULL, "pp-dbg-b-modes", 1,
                                                 "Display only selected block modes");
static const arg_def_t pp_disp_mvs = ARG_DEF(NULL, "pp-dbg-mvs", 1,
                                             "Draw only selected motion vectors");
static const arg_def_t mfqe = ARG_DEF(NULL, "mfqe", 0,
                                      "Enable multiframe quality enhancement");

static const arg_def_t *vp8_pp_args[] = {
  &addnoise_level, &deblock, &demacroblock_level, &pp_debug_info,
  &pp_disp_ref_frame, &pp_disp_mb_modes, &pp_disp_b_modes, &pp_disp_mvs, &mfqe,
  NULL
};
#endif

static void usage_exit() {
  int i;

  fprintf(stderr, "Usage: %s <options> filename\n\n"
          "Options:\n", exec_name);
  arg_show_usage(stderr, all_args);
#if CONFIG_VP8_DECODER
  fprintf(stderr, "\nVP8 Postprocessing Options:\n");
  arg_show_usage(stderr, vp8_pp_args);
#endif
  fprintf(stderr,
          "\nOutput File Patterns:\n\n"
          "  The -o argument specifies the name of the file(s) to "
          "write to. If the\n  argument does not include any escape "
          "characters, the output will be\n  written to a single file. "
          "Otherwise, the filename will be calculated by\n  expanding "
          "the following escape characters:\n");
  fprintf(stderr,
          "\n\t%%w   - Frame width"
          "\n\t%%h   - Frame height"
          "\n\t%%<n> - Frame number, zero padded to <n> places (1..9)"
          "\n\n  Pattern arguments are only supported in conjunction "
          "with the --yv12 and\n  --i420 options. If the -o option is "
          "not specified, the output will be\n  directed to stdout.\n"
         );
  fprintf(stderr, "\nIncluded decoders:\n\n");

  for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++)
    fprintf(stderr, "    %-6s - %s\n",
            ifaces[i].name,
            vpx_codec_iface_name(ifaces[i].iface()));

  exit(EXIT_FAILURE);
}

void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  usage_exit();
}

static unsigned int mem_get_le16(const void *vmem) {
  unsigned int  val;
  const unsigned char *mem = (const unsigned char *)vmem;

  val = mem[1] << 8;
  val |= mem[0];
  return val;
}

static unsigned int mem_get_le32(const void *vmem) {
  unsigned int  val;
  const unsigned char *mem = (const unsigned char *)vmem;

  val = mem[3] << 24;
  val |= mem[2] << 16;
  val |= mem[1] << 8;
  val |= mem[0];
  return val;
}

enum file_kind {
  RAW_FILE,
  IVF_FILE,
  WEBM_FILE
};

struct input_ctx {
  enum file_kind  kind;
  FILE           *infile;
  nestegg        *nestegg_ctx;
  nestegg_packet *pkt;
  unsigned int    chunk;
  unsigned int    chunks;
  unsigned int    video_track;
};

#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))
static int read_frame(struct input_ctx      *input,
                      uint8_t               **buf,
                      size_t                *buf_sz,
                      size_t                *buf_alloc_sz) {
  char            raw_hdr[IVF_FRAME_HDR_SZ];
  size_t          new_buf_sz;
  FILE           *infile = input->infile;
  enum file_kind  kind = input->kind;
  if (kind == WEBM_FILE) {
    if (input->chunk >= input->chunks) {
      unsigned int track;

      do {
        /* End of this packet, get another. */
        if (input->pkt)
          nestegg_free_packet(input->pkt);

        if (nestegg_read_packet(input->nestegg_ctx, &input->pkt) <= 0
            || nestegg_packet_track(input->pkt, &track))
          return 1;

      } while (track != input->video_track);

      if (nestegg_packet_count(input->pkt, &input->chunks))
        return 1;
      input->chunk = 0;
    }

    if (nestegg_packet_data(input->pkt, input->chunk, buf, buf_sz))
      return 1;
    input->chunk++;

    return 0;
  }
  /* For both the raw and ivf formats, the frame size is the first 4 bytes
   * of the frame header. We just need to special case on the header
   * size.
   */
  else if (fread(raw_hdr, kind == IVF_FILE
                 ? IVF_FRAME_HDR_SZ : RAW_FRAME_HDR_SZ, 1, infile) != 1) {
    if (!feof(infile))
      fprintf(stderr, "Failed to read frame size\n");

    new_buf_sz = 0;
  } else {
    new_buf_sz = mem_get_le32(raw_hdr);

    if (new_buf_sz > 256 * 1024 * 1024) {
      fprintf(stderr, "Error: Read invalid frame size (%u)\n",
              (unsigned int)new_buf_sz);
      new_buf_sz = 0;
    }

    if (kind == RAW_FILE && new_buf_sz > 256 * 1024)
      fprintf(stderr, "Warning: Read invalid frame size (%u)"
              " - not a raw file?\n", (unsigned int)new_buf_sz);

    if (new_buf_sz > *buf_alloc_sz) {
      uint8_t *new_buf = realloc(*buf, 2 * new_buf_sz);

      if (new_buf) {
        *buf = new_buf;
        *buf_alloc_sz = 2 * new_buf_sz;
      } else {
        fprintf(stderr, "Failed to allocate compressed data buffer\n");
        new_buf_sz = 0;
      }
    }
  }

  *buf_sz = new_buf_sz;

  if (!feof(infile)) {
    if (fread(*buf, 1, *buf_sz, infile) != *buf_sz) {
      fprintf(stderr, "Failed to read full frame\n");
      return 1;
    }

    return 0;
  }

  return 1;
}

void *out_open(const char *out_fn, int do_md5) {
  void *out = NULL;

  if (do_md5) {
#if CONFIG_MD5
    MD5Context *md5_ctx = out = malloc(sizeof(MD5Context));
    (void)out_fn;
    MD5Init(md5_ctx);
#endif
  } else {
    FILE *outfile = out = strcmp("-", out_fn) ? fopen(out_fn, "wb")
                          : set_binary_mode(stdout);

    if (!outfile) {
      fprintf(stderr, "Failed to output file");
      exit(EXIT_FAILURE);
    }
  }

  return out;
}

void out_put(void *out, const uint8_t *buf, unsigned int len, int do_md5) {
  if (do_md5) {
#if CONFIG_MD5
    MD5Update(out, buf, len);
#endif
  } else {
    (void) fwrite(buf, 1, len, out);
  }
}

void out_close(void *out, const char *out_fn, int do_md5) {
  if (do_md5) {
#if CONFIG_MD5
    uint8_t md5[16];
    int i;

    MD5Final(md5, out);
    free(out);

    for (i = 0; i < 16; i++)
      printf("%02x", md5[i]);

    printf("  %s\n", out_fn);
#endif
  } else {
    fclose(out);
  }
}

unsigned int file_is_ivf(FILE *infile,
                         unsigned int *fourcc,
                         unsigned int *width,
                         unsigned int *height,
                         unsigned int *fps_den,
                         unsigned int *fps_num) {
  char raw_hdr[32];
  int is_ivf = 0;

  if (fread(raw_hdr, 1, 32, infile) == 32) {
    if (raw_hdr[0] == 'D' && raw_hdr[1] == 'K'
        && raw_hdr[2] == 'I' && raw_hdr[3] == 'F') {
      is_ivf = 1;

      if (mem_get_le16(raw_hdr + 4) != 0)
        fprintf(stderr, "Error: Unrecognized IVF version! This file may not"
                " decode properly.");

      *fourcc = mem_get_le32(raw_hdr + 8);
      *width = mem_get_le16(raw_hdr + 12);
      *height = mem_get_le16(raw_hdr + 14);
      *fps_num = mem_get_le32(raw_hdr + 16);
      *fps_den = mem_get_le32(raw_hdr + 20);

      /* Some versions of vpxenc used 1/(2*fps) for the timebase, so
       * we can guess the framerate using only the timebase in this
       * case. Other files would require reading ahead to guess the
       * timebase, like we do for webm.
       */
      if (*fps_num < 1000) {
        /* Correct for the factor of 2 applied to the timebase in the
         * encoder.
         */
        if (*fps_num & 1)*fps_den <<= 1;
        else *fps_num >>= 1;
      } else {
        /* Don't know FPS for sure, and don't have readahead code
         * (yet?), so just default to 30fps.
         */
        *fps_num = 30;
        *fps_den = 1;
      }
    }
  }

  if (!is_ivf)
    rewind(infile);

  return is_ivf;
}


unsigned int file_is_raw(FILE *infile,
                         unsigned int *fourcc,
                         unsigned int *width,
                         unsigned int *height,
                         unsigned int *fps_den,
                         unsigned int *fps_num) {
  unsigned char buf[32];
  int is_raw = 0;
  vpx_codec_stream_info_t si;

  si.sz = sizeof(si);

  if (fread(buf, 1, 32, infile) == 32) {
    int i;

    if (mem_get_le32(buf) < 256 * 1024 * 1024)
      for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++)
        if (!vpx_codec_peek_stream_info(ifaces[i].iface(),
                                        buf + 4, 32 - 4, &si)) {
          is_raw = 1;
          *fourcc = ifaces[i].fourcc;
          *width = si.w;
          *height = si.h;
          *fps_num = 30;
          *fps_den = 1;
          break;
        }
  }

  rewind(infile);
  return is_raw;
}


static int
nestegg_read_cb(void *buffer, size_t length, void *userdata) {
  FILE *f = userdata;

  if (fread(buffer, 1, length, f) < length) {
    if (ferror(f))
      return -1;
    if (feof(f))
      return 0;
  }
  return 1;
}


static int
nestegg_seek_cb(int64_t offset, int whence, void *userdata) {
  switch (whence) {
    case NESTEGG_SEEK_SET:
      whence = SEEK_SET;
      break;
    case NESTEGG_SEEK_CUR:
      whence = SEEK_CUR;
      break;
    case NESTEGG_SEEK_END:
      whence = SEEK_END;
      break;
  };
  return fseek(userdata, (long)offset, whence) ? -1 : 0;
}


static int64_t
nestegg_tell_cb(void *userdata) {
  return ftell(userdata);
}


static void
nestegg_log_cb(nestegg *context, unsigned int severity, char const *format,
               ...) {
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}


static int
webm_guess_framerate(struct input_ctx *input,
                     unsigned int     *fps_den,
                     unsigned int     *fps_num) {
  unsigned int i;
  uint64_t     tstamp = 0;

  /* Guess the framerate. Read up to 1 second, or 50 video packets,
   * whichever comes first.
   */
  for (i = 0; tstamp < 1000000000 && i < 50;) {
    nestegg_packet *pkt;
    unsigned int track;

    if (nestegg_read_packet(input->nestegg_ctx, &pkt) <= 0)
      break;

    nestegg_packet_track(pkt, &track);
    if (track == input->video_track) {
      nestegg_packet_tstamp(pkt, &tstamp);
      i++;
    }

    nestegg_free_packet(pkt);
  }

  if (nestegg_track_seek(input->nestegg_ctx, input->video_track, 0))
    goto fail;

  *fps_num = (i - 1) * 1000000;
  *fps_den = (unsigned int)(tstamp / 1000);
  return 0;
fail:
  nestegg_destroy(input->nestegg_ctx);
  input->nestegg_ctx = NULL;
  rewind(input->infile);
  return 1;
}


static int
file_is_webm(struct input_ctx *input,
             unsigned int     *fourcc,
             unsigned int     *width,
             unsigned int     *height,
             unsigned int     *fps_den,
             unsigned int     *fps_num) {
  unsigned int i, n;
  int          track_type = -1;
  int          codec_id;

  nestegg_io io = {nestegg_read_cb, nestegg_seek_cb, nestegg_tell_cb, 0};
  nestegg_video_params params;

  io.userdata = input->infile;
  if (nestegg_init(&input->nestegg_ctx, io, NULL))
    goto fail;

  if (nestegg_track_count(input->nestegg_ctx, &n))
    goto fail;

  for (i = 0; i < n; i++) {
    track_type = nestegg_track_type(input->nestegg_ctx, i);

    if (track_type == NESTEGG_TRACK_VIDEO)
      break;
    else if (track_type < 0)
      goto fail;
  }

  codec_id = nestegg_track_codec_id(input->nestegg_ctx, i);
  if (codec_id == NESTEGG_CODEC_VP8) {
    *fourcc = VP8_FOURCC;
  } else if (codec_id == NESTEGG_CODEC_VP9) {
    *fourcc = VP9_FOURCC;
  } else {
    fprintf(stderr, "Not VPx video, quitting.\n");
    exit(1);
  }

  input->video_track = i;

  if (nestegg_track_video_params(input->nestegg_ctx, i, &params))
    goto fail;

  *fps_den = 0;
  *fps_num = 0;
  *width = params.width;
  *height = params.height;
  return 1;
fail:
  input->nestegg_ctx = NULL;
  rewind(input->infile);
  return 0;
}


void show_progress(int frame_in, int frame_out, unsigned long dx_time) {
  fprintf(stderr, "%d decoded frames/%d showed frames in %lu us (%.2f fps)\r",
          frame_in, frame_out, dx_time,
          (float)frame_out * 1000000.0 / (float)dx_time);
}


void generate_filename(const char *pattern, char *out, size_t q_len,
                       unsigned int d_w, unsigned int d_h,
                       unsigned int frame_in) {
  const char *p = pattern;
  char *q = out;

  do {
    char *next_pat = strchr(p, '%');

    if (p == next_pat) {
      size_t pat_len;

      /* parse the pattern */
      q[q_len - 1] = '\0';
      switch (p[1]) {
        case 'w':
          snprintf(q, q_len - 1, "%d", d_w);
          break;
        case 'h':
          snprintf(q, q_len - 1, "%d", d_h);
          break;
        case '1':
          snprintf(q, q_len - 1, "%d", frame_in);
          break;
        case '2':
          snprintf(q, q_len - 1, "%02d", frame_in);
          break;
        case '3':
          snprintf(q, q_len - 1, "%03d", frame_in);
          break;
        case '4':
          snprintf(q, q_len - 1, "%04d", frame_in);
          break;
        case '5':
          snprintf(q, q_len - 1, "%05d", frame_in);
          break;
        case '6':
          snprintf(q, q_len - 1, "%06d", frame_in);
          break;
        case '7':
          snprintf(q, q_len - 1, "%07d", frame_in);
          break;
        case '8':
          snprintf(q, q_len - 1, "%08d", frame_in);
          break;
        case '9':
          snprintf(q, q_len - 1, "%09d", frame_in);
          break;
        default:
          die("Unrecognized pattern %%%c\n", p[1]);
      }

      pat_len = strlen(q);
      if (pat_len >= q_len - 1)
        die("Output filename too long.\n");
      q += pat_len;
      p += 2;
      q_len -= pat_len;
    } else {
      size_t copy_len;

      /* copy the next segment */
      if (!next_pat)
        copy_len = strlen(p);
      else
        copy_len = next_pat - p;

      if (copy_len >= q_len - 1)
        die("Output filename too long.\n");

      memcpy(q, p, copy_len);
      q[copy_len] = '\0';
      q += copy_len;
      p += copy_len;
      q_len -= copy_len;
    }
  } while (*p);
}


int main(int argc, const char **argv_) {
  vpx_codec_ctx_t          decoder;
  char                  *fn = NULL;
  int                    i;
  uint8_t               *buf = NULL;
  size_t                 buf_sz = 0, buf_alloc_sz = 0;
  FILE                  *infile;
  int                    frame_in = 0, frame_out = 0, flipuv = 0, noblit = 0, do_md5 = 0, progress = 0;
  int                    stop_after = 0, postproc = 0, summary = 0, quiet = 1;
  int                    arg_skip = 0;
  int                    ec_enabled = 0;
  vpx_codec_iface_t       *iface = NULL;
  unsigned int           fourcc;
  unsigned long          dx_time = 0;
  struct arg               arg;
  char                   **argv, **argi, **argj;
  const char             *outfile_pattern = 0;
  char                    outfile[PATH_MAX];
  int                     single_file;
  int                     use_y4m = 1;
  unsigned int            width;
  unsigned int            height;
  unsigned int            fps_den;
  unsigned int            fps_num;
  void                   *out = NULL;
  vpx_codec_dec_cfg_t     cfg = {0};
#if CONFIG_VP8_DECODER
  vp8_postproc_cfg_t      vp8_pp_cfg = {0};
  int                     vp8_dbg_color_ref_frame = 0;
  int                     vp8_dbg_color_mb_modes = 0;
  int                     vp8_dbg_color_b_modes = 0;
  int                     vp8_dbg_display_mv = 0;
#endif
  struct input_ctx        input = {0};
  int                     frames_corrupted = 0;
  int                     dec_flags = 0;
  int                     do_scale = 0;
  int                     stream_w = 0, stream_h = 0;
  vpx_image_t             *scaled_img = NULL;

  /* Parse command line */
  exec_name = argv_[0];
  argv = argv_dup(argc - 1, argv_ + 1);

  for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step) {
    memset(&arg, 0, sizeof(arg));
    arg.argv_step = 1;

    if (arg_match(&arg, &codecarg, argi)) {
      int j, k = -1;

      for (j = 0; j < sizeof(ifaces) / sizeof(ifaces[0]); j++)
        if (!strcmp(ifaces[j].name, arg.val))
          k = j;

      if (k >= 0)
        iface = ifaces[k].iface();
      else
        die("Error: Unrecognized argument (%s) to --codec\n",
            arg.val);
    } else if (arg_match(&arg, &outputfile, argi))
      outfile_pattern = arg.val;
    else if (arg_match(&arg, &use_yv12, argi)) {
      use_y4m = 0;
      flipuv = 1;
    } else if (arg_match(&arg, &use_i420, argi)) {
      use_y4m = 0;
      flipuv = 0;
    } else if (arg_match(&arg, &flipuvarg, argi))
      flipuv = 1;
    else if (arg_match(&arg, &noblitarg, argi))
      noblit = 1;
    else if (arg_match(&arg, &progressarg, argi))
      progress = 1;
    else if (arg_match(&arg, &limitarg, argi))
      stop_after = arg_parse_uint(&arg);
    else if (arg_match(&arg, &skiparg, argi))
      arg_skip = arg_parse_uint(&arg);
    else if (arg_match(&arg, &postprocarg, argi))
      postproc = 1;
    else if (arg_match(&arg, &md5arg, argi))
      do_md5 = 1;
    else if (arg_match(&arg, &summaryarg, argi))
      summary = 1;
    else if (arg_match(&arg, &threadsarg, argi))
      cfg.threads = arg_parse_uint(&arg);
    else if (arg_match(&arg, &verbosearg, argi))
      quiet = 0;
    else if (arg_match(&arg, &scalearg, argi))
      do_scale = 1;

#if CONFIG_VP8_DECODER
    else if (arg_match(&arg, &addnoise_level, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_ADDNOISE;
      vp8_pp_cfg.noise_level = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &demacroblock_level, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_DEMACROBLOCK;
      vp8_pp_cfg.deblocking_level = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &deblock, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_DEBLOCK;
    } else if (arg_match(&arg, &mfqe, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_MFQE;
    } else if (arg_match(&arg, &pp_debug_info, argi)) {
      unsigned int level = arg_parse_uint(&arg);

      postproc = 1;
      vp8_pp_cfg.post_proc_flag &= ~0x7;

      if (level)
        vp8_pp_cfg.post_proc_flag |= level;
    } else if (arg_match(&arg, &pp_disp_ref_frame, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_color_ref_frame = flags;
      }
    } else if (arg_match(&arg, &pp_disp_mb_modes, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_color_mb_modes = flags;
      }
    } else if (arg_match(&arg, &pp_disp_b_modes, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_color_b_modes = flags;
      }
    } else if (arg_match(&arg, &pp_disp_mvs, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_display_mv = flags;
      }
    } else if (arg_match(&arg, &error_concealment, argi)) {
      ec_enabled = 1;
    }

#endif
    else
      argj++;
  }

  /* Check for unrecognized options */
  for (argi = argv; *argi; argi++)
    if (argi[0][0] == '-' && strlen(argi[0]) > 1)
      die("Error: Unrecognized option %s\n", *argi);

  /* Handle non-option arguments */
  fn = argv[0];

  if (!fn)
    usage_exit();

  /* Open file */
  infile = strcmp(fn, "-") ? fopen(fn, "rb") : set_binary_mode(stdin);

  if (!infile) {
    fprintf(stderr, "Failed to open file '%s'",
            strcmp(fn, "-") ? fn : "stdin");
    return EXIT_FAILURE;
  }
#if CONFIG_OS_SUPPORT
  /* Make sure we don't dump to the terminal, unless forced to with -o - */
  if (!outfile_pattern && isatty(fileno(stdout)) && !do_md5 && !noblit) {
    fprintf(stderr,
            "Not dumping raw video to your terminal. Use '-o -' to "
            "override.\n");
    return EXIT_FAILURE;
  }
#endif
  input.infile = infile;
  if (file_is_ivf(infile, &fourcc, &width, &height, &fps_den,
                  &fps_num))
    input.kind = IVF_FILE;
  else if (file_is_webm(&input, &fourcc, &width, &height, &fps_den, &fps_num))
    input.kind = WEBM_FILE;
  else if (file_is_raw(infile, &fourcc, &width, &height, &fps_den, &fps_num))
    input.kind = RAW_FILE;
  else {
    fprintf(stderr, "Unrecognized input file type.\n");
    return EXIT_FAILURE;
  }

  /* If the output file is not set or doesn't have a sequence number in
   * it, then we only open it once.
   */
  outfile_pattern = outfile_pattern ? outfile_pattern : "-";
  single_file = 1;
  {
    const char *p = outfile_pattern;
    do {
      p = strchr(p, '%');
      if (p && p[1] >= '1' && p[1] <= '9') {
        /* pattern contains sequence number, so it's not unique. */
        single_file = 0;
        break;
      }
      if (p)
        p++;
    } while (p);
  }

  if (single_file && !noblit) {
    generate_filename(outfile_pattern, outfile, sizeof(outfile) - 1,
                      width, height, 0);
    out = out_open(outfile, do_md5);
  }

  if (use_y4m && !noblit) {
    char buffer[128];
    if (!single_file) {
      fprintf(stderr, "YUV4MPEG2 not supported with output patterns,"
              " try --i420 or --yv12.\n");
      return EXIT_FAILURE;
    }

    if (input.kind == WEBM_FILE)
      if (webm_guess_framerate(&input, &fps_den, &fps_num)) {
        fprintf(stderr, "Failed to guess framerate -- error parsing "
                "webm file?\n");
        return EXIT_FAILURE;
      }


    /*Note: We can't output an aspect ratio here because IVF doesn't
       store one, and neither does VP8.
      That will have to wait until these tools support WebM natively.*/
    sprintf(buffer, "YUV4MPEG2 C%s W%u H%u F%u:%u I%c\n",
            "420jpeg", width, height, fps_num, fps_den, 'p');
    out_put(out, (unsigned char *)buffer,
            (unsigned int)strlen(buffer), do_md5);
  }

  /* Try to determine the codec from the fourcc. */
  for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++)
    if ((fourcc & ifaces[i].fourcc_mask) == ifaces[i].fourcc) {
      vpx_codec_iface_t  *ivf_iface = ifaces[i].iface();

      if (iface && iface != ivf_iface)
        fprintf(stderr, "Notice -- IVF header indicates codec: %s\n",
                ifaces[i].name);
      else
        iface = ivf_iface;

      break;
    }

  dec_flags = (postproc ? VPX_CODEC_USE_POSTPROC : 0) |
              (ec_enabled ? VPX_CODEC_USE_ERROR_CONCEALMENT : 0);
  if (vpx_codec_dec_init(&decoder, iface ? iface :  ifaces[0].iface(), &cfg,
                         dec_flags)) {
    fprintf(stderr, "Failed to initialize decoder: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (!quiet)
    fprintf(stderr, "%s\n", decoder.name);

#if CONFIG_VP8_DECODER

  if (vp8_pp_cfg.post_proc_flag
      && vpx_codec_control(&decoder, VP8_SET_POSTPROC, &vp8_pp_cfg)) {
    fprintf(stderr, "Failed to configure postproc: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_color_ref_frame
      && vpx_codec_control(&decoder, VP8_SET_DBG_COLOR_REF_FRAME, vp8_dbg_color_ref_frame)) {
    fprintf(stderr, "Failed to configure reference block visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_color_mb_modes
      && vpx_codec_control(&decoder, VP8_SET_DBG_COLOR_MB_MODES, vp8_dbg_color_mb_modes)) {
    fprintf(stderr, "Failed to configure macro block visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_color_b_modes
      && vpx_codec_control(&decoder, VP8_SET_DBG_COLOR_B_MODES, vp8_dbg_color_b_modes)) {
    fprintf(stderr, "Failed to configure block visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_display_mv
      && vpx_codec_control(&decoder, VP8_SET_DBG_DISPLAY_MV, vp8_dbg_display_mv)) {
    fprintf(stderr, "Failed to configure motion vector visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }
#endif


  if(arg_skip)
    fprintf(stderr, "Skiping first %d frames.\n", arg_skip);
  while (arg_skip) {
    if (read_frame(&input, &buf, &buf_sz, &buf_alloc_sz))
      break;
    arg_skip--;
  }

  /* Decode file */
  while (!read_frame(&input, &buf, &buf_sz, &buf_alloc_sz)) {
    vpx_codec_iter_t  iter = NULL;
    vpx_image_t    *img;
    struct vpx_usec_timer timer;
    int                   corrupted;

    vpx_usec_timer_start(&timer);

    if (vpx_codec_decode(&decoder, buf, (unsigned int)buf_sz, NULL, 0)) {
      const char *detail = vpx_codec_error_detail(&decoder);
      fprintf(stderr, "Failed to decode frame: %s\n", vpx_codec_error(&decoder));

      if (detail)
        fprintf(stderr, "  Additional information: %s\n", detail);

      goto fail;
    }

    vpx_usec_timer_mark(&timer);
    dx_time += (unsigned int)vpx_usec_timer_elapsed(&timer);

    ++frame_in;

    if (vpx_codec_control(&decoder, VP8D_GET_FRAME_CORRUPTED, &corrupted)) {
      fprintf(stderr, "Failed VP8_GET_FRAME_CORRUPTED: %s\n",
              vpx_codec_error(&decoder));
      goto fail;
    }
    frames_corrupted += corrupted;

    vpx_usec_timer_start(&timer);

    if ((img = vpx_codec_get_frame(&decoder, &iter)))
      ++frame_out;

    vpx_usec_timer_mark(&timer);
    dx_time += (unsigned int)vpx_usec_timer_elapsed(&timer);

    if (progress)
      show_progress(frame_in, frame_out, dx_time);

    if (!noblit) {
      if (do_scale) {
        if (img && frame_out == 1) {
          stream_w = img->d_w;
          stream_h = img->d_h;
          scaled_img = vpx_img_alloc(NULL, VPX_IMG_FMT_I420,
                                     stream_w, stream_h, 16);
        }
        if (img && (img->d_w != stream_w || img->d_h != stream_h)) {
          I420Scale(img->planes[VPX_PLANE_Y], img->stride[VPX_PLANE_Y],
                    img->planes[VPX_PLANE_U], img->stride[VPX_PLANE_U],
                    img->planes[VPX_PLANE_V], img->stride[VPX_PLANE_V],
                    img->d_w, img->d_h,
                    scaled_img->planes[VPX_PLANE_Y],
                    scaled_img->stride[VPX_PLANE_Y],
                    scaled_img->planes[VPX_PLANE_U],
                    scaled_img->stride[VPX_PLANE_U],
                    scaled_img->planes[VPX_PLANE_V],
                    scaled_img->stride[VPX_PLANE_V],
                    stream_w, stream_h,
                    kFilterBox);
          img = scaled_img;
        }
      }

      if (img) {
        unsigned int y;
        char out_fn[PATH_MAX];
        uint8_t *buf;

        if (!single_file) {
          size_t len = sizeof(out_fn) - 1;

          out_fn[len] = '\0';
          generate_filename(outfile_pattern, out_fn, len - 1,
                            img->d_w, img->d_h, frame_in);
          out = out_open(out_fn, do_md5);
        } else if (use_y4m)
          out_put(out, (unsigned char *)"FRAME\n", 6, do_md5);

        buf = img->planes[VPX_PLANE_Y];

        for (y = 0; y < img->d_h; y++) {
          out_put(out, buf, img->d_w, do_md5);
          buf += img->stride[VPX_PLANE_Y];
        }

        buf = img->planes[flipuv ? VPX_PLANE_V : VPX_PLANE_U];

        for (y = 0; y < (1 + img->d_h) / 2; y++) {
          out_put(out, buf, (1 + img->d_w) / 2, do_md5);
          buf += img->stride[VPX_PLANE_U];
        }

        buf = img->planes[flipuv ? VPX_PLANE_U : VPX_PLANE_V];

        for (y = 0; y < (1 + img->d_h) / 2; y++) {
          out_put(out, buf, (1 + img->d_w) / 2, do_md5);
          buf += img->stride[VPX_PLANE_V];
        }

        if (!single_file)
          out_close(out, out_fn, do_md5);
      }
    }

    if (stop_after && frame_in >= stop_after)
      break;
  }

  if (summary || progress) {
    show_progress(frame_in, frame_out, dx_time);
    fprintf(stderr, "\n");
  }

  if (frames_corrupted)
    fprintf(stderr, "WARNING: %d frames corrupted.\n", frames_corrupted);

fail:

  if (vpx_codec_destroy(&decoder)) {
    fprintf(stderr, "Failed to destroy decoder: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (single_file && !noblit)
    out_close(out, outfile, do_md5);

  if (input.nestegg_ctx)
    nestegg_destroy(input.nestegg_ctx);
  if (input.kind != WEBM_FILE)
    free(buf);
  fclose(infile);
  free(argv);

  return frames_corrupted ? EXIT_FAILURE : EXIT_SUCCESS;
}
