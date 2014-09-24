/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*!\defgroup vp8_encoder WebM VP8 Encoder
 * \ingroup vp8
 *
 * @{
 */
#include "vp8.h"

/*!\file
 * \brief Provides definitions for using the VP8 encoder algorithm within the
 *        vpx Codec Interface.
 */
#ifndef VP8CX_H
#define VP8CX_H
#include "vpx_codec_impl_top.h"

/*!\name Algorithm interface for VP8
 *
 * This interface provides the capability to encode raw VP8 streams, as would
 * be found in AVI files.
 * @{
 */
extern vpx_codec_iface_t  vpx_codec_vp8_cx_algo;
extern vpx_codec_iface_t *vpx_codec_vp8_cx(void);

/* TODO(jkoleszar): These move to VP9 in a later patch set. */
extern vpx_codec_iface_t  vpx_codec_vp9_cx_algo;
extern vpx_codec_iface_t *vpx_codec_vp9_cx(void);
extern vpx_codec_iface_t  vpx_codec_vp9x_cx_algo;
extern vpx_codec_iface_t *vpx_codec_vp9x_cx(void);

/*!@} - end algorithm interface member group*/


/*
 * Algorithm Flags
 */

/*!\brief Don't reference the last frame
 *
 * When this flag is set, the encoder will not use the last frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * last frame or not automatically.
 */
#define VP8_EFLAG_NO_REF_LAST      (1<<16)


/*!\brief Don't reference the golden frame
 *
 * When this flag is set, the encoder will not use the golden frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * golden frame or not automatically.
 */
#define VP8_EFLAG_NO_REF_GF        (1<<17)


/*!\brief Don't reference the alternate reference frame
 *
 * When this flag is set, the encoder will not use the alt ref frame as a
 * predictor. When not set, the encoder will choose whether to use the
 * alt ref frame or not automatically.
 */
#define VP8_EFLAG_NO_REF_ARF       (1<<21)


/*!\brief Don't update the last frame
 *
 * When this flag is set, the encoder will not update the last frame with
 * the contents of the current frame.
 */
#define VP8_EFLAG_NO_UPD_LAST      (1<<18)


/*!\brief Don't update the golden frame
 *
 * When this flag is set, the encoder will not update the golden frame with
 * the contents of the current frame.
 */
#define VP8_EFLAG_NO_UPD_GF        (1<<22)


/*!\brief Don't update the alternate reference frame
 *
 * When this flag is set, the encoder will not update the alt ref frame with
 * the contents of the current frame.
 */
#define VP8_EFLAG_NO_UPD_ARF       (1<<23)


/*!\brief Force golden frame update
 *
 * When this flag is set, the encoder copy the contents of the current frame
 * to the golden frame buffer.
 */
#define VP8_EFLAG_FORCE_GF         (1<<19)


/*!\brief Force alternate reference frame update
 *
 * When this flag is set, the encoder copy the contents of the current frame
 * to the alternate reference frame buffer.
 */
#define VP8_EFLAG_FORCE_ARF        (1<<24)


/*!\brief Disable entropy update
 *
 * When this flag is set, the encoder will not update its internal entropy
 * model based on the entropy of this frame.
 */
#define VP8_EFLAG_NO_UPD_ENTROPY   (1<<20)


/*!\brief VP8 encoder control functions
 *
 * This set of macros define the control functions available for the VP8
 * encoder interface.
 *
 * \sa #vpx_codec_control
 */
enum vp8e_enc_control_id {
  VP8E_UPD_ENTROPY           = 5,  /**< control function to set mode of entropy update in encoder */
  VP8E_UPD_REFERENCE,              /**< control function to set reference update mode in encoder */
  VP8E_USE_REFERENCE,              /**< control function to set which reference frame encoder can use */
  VP8E_SET_ROI_MAP,                /**< control function to pass an ROI map to encoder */
  VP8E_SET_ACTIVEMAP,              /**< control function to pass an Active map to encoder */
  VP8E_SET_SCALEMODE         = 11, /**< control function to set encoder scaling mode */
  /*!\brief control function to set vp8 encoder cpuused
   *
   * Changes in this value influences, among others, the encoder's selection
   * of motion estimation methods. Values greater than 0 will increase encoder
   * speed at the expense of quality.
   * The full set of adjustments can be found in
   * onyx_if.c:vp8_set_speed_features().
   * \todo List highlights of the changes at various levels.
   *
   * \note Valid range: -16..16
   */
  VP8E_SET_CPUUSED           = 13,
  VP8E_SET_ENABLEAUTOALTREF,       /**< control function to enable vp8 to automatic set and use altref frame */
  VP8E_SET_NOISE_SENSITIVITY,      /**< control function to set noise sensitivity */
  VP8E_SET_SHARPNESS,              /**< control function to set sharpness */
  VP8E_SET_STATIC_THRESHOLD,       /**< control function to set the threshold for macroblocks treated static */
  VP8E_SET_TOKEN_PARTITIONS,       /**< control function to set the number of token partitions  */
  VP8E_GET_LAST_QUANTIZER,         /**< return the quantizer chosen by the
                                          encoder for the last frame using the internal
                                          scale */
  VP8E_GET_LAST_QUANTIZER_64,      /**< return the quantizer chosen by the
                                          encoder for the last frame, using the 0..63
                                          scale as used by the rc_*_quantizer config
                                          parameters */
  VP8E_SET_ARNR_MAXFRAMES,         /**< control function to set the max number of frames blurred creating arf*/
  VP8E_SET_ARNR_STRENGTH,         /**< control function to set the filter strength for the arf */
  VP8E_SET_ARNR_TYPE,         /**< control function to set the type of filter to use for the arf*/
  VP8E_SET_TUNING,                 /**< control function to set visual tuning */
  /*!\brief control function to set constrained quality level
   *
   * \attention For this value to be used vpx_codec_enc_cfg_t::g_usage must be
   *            set to #VPX_CQ.
   * \note Valid range: 0..63
   */
  VP8E_SET_CQ_LEVEL,

  /*!\brief Max data rate for Intra frames
   *
   * This value controls additional clamping on the maximum size of a
   * keyframe. It is expressed as a percentage of the average
   * per-frame bitrate, with the special (and default) value 0 meaning
   * unlimited, or no additional clamping beyond the codec's built-in
   * algorithm.
   *
   * For example, to allocate no more than 4.5 frames worth of bitrate
   * to a keyframe, set this to 450.
   *
   */
  VP8E_SET_MAX_INTRA_BITRATE_PCT,


  /* TODO(jkoleszar): Move to vp9cx.h */
  VP9E_SET_LOSSLESS
};

/*!\brief vpx 1-D scaling mode
 *
 * This set of constants define 1-D vpx scaling modes
 */
typedef enum vpx_scaling_mode_1d {
  VP8E_NORMAL      = 0,
  VP8E_FOURFIVE    = 1,
  VP8E_THREEFIVE   = 2,
  VP8E_ONETWO      = 3
} VPX_SCALING_MODE;


/*!\brief  vpx region of interest map
 *
 * These defines the data structures for the region of interest map
 *
 */

typedef struct vpx_roi_map {
  unsigned char *roi_map;      /**< specify an id between 0 and 3 for each 16x16 region within a frame */
  unsigned int   rows;         /**< number of rows */
  unsigned int   cols;         /**< number of cols */
  int     delta_q[4];          /**< quantizer delta [-63, 63] off baseline for regions with id between 0 and 3*/
  int     delta_lf[4];         /**< loop filter strength delta [-63, 63] for regions with id between 0 and 3 */
  unsigned int   static_threshold[4];/**< threshold for region to be treated as static */
} vpx_roi_map_t;

/*!\brief  vpx active region map
 *
 * These defines the data structures for active region map
 *
 */


typedef struct vpx_active_map {
  unsigned char  *active_map; /**< specify an on (1) or off (0) each 16x16 region within a frame */
  unsigned int    rows;       /**< number of rows */
  unsigned int    cols;       /**< number of cols */
} vpx_active_map_t;

/*!\brief  vpx image scaling mode
 *
 * This defines the data structure for image scaling mode
 *
 */
typedef struct vpx_scaling_mode {
  VPX_SCALING_MODE    h_scaling_mode;  /**< horizontal scaling mode */
  VPX_SCALING_MODE    v_scaling_mode;  /**< vertical scaling mode   */
} vpx_scaling_mode_t;

/*!\brief VP8 token partition mode
 *
 * This defines VP8 partitioning mode for compressed data, i.e., the number of
 * sub-streams in the bitstream. Used for parallelized decoding.
 *
 */

typedef enum {
  VP8_ONE_TOKENPARTITION   = 0,
  VP8_TWO_TOKENPARTITION   = 1,
  VP8_FOUR_TOKENPARTITION  = 2,
  VP8_EIGHT_TOKENPARTITION = 3
} vp8e_token_partitions;


/*!\brief VP8 model tuning parameters
 *
 * Changes the encoder to tune for certain types of input material.
 *
 */
typedef enum {
  VP8_TUNE_PSNR,
  VP8_TUNE_SSIM
} vp8e_tuning;


/*!\brief VP8 encoder control function parameter type
 *
 * Defines the data types that VP8E control functions take. Note that
 * additional common controls are defined in vp8.h
 *
 */


/* These controls have been deprecated in favor of the flags parameter to
 * vpx_codec_encode(). See the definition of VP8_EFLAG_* above.
 */
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_UPD_ENTROPY,            int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_UPD_REFERENCE,          int)
VPX_CTRL_USE_TYPE_DEPRECATED(VP8E_USE_REFERENCE,          int)

VPX_CTRL_USE_TYPE(VP8E_SET_ROI_MAP,            vpx_roi_map_t *)
VPX_CTRL_USE_TYPE(VP8E_SET_ACTIVEMAP,          vpx_active_map_t *)
VPX_CTRL_USE_TYPE(VP8E_SET_SCALEMODE,          vpx_scaling_mode_t *)

VPX_CTRL_USE_TYPE(VP8E_SET_CPUUSED,            int)
VPX_CTRL_USE_TYPE(VP8E_SET_ENABLEAUTOALTREF,   unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_NOISE_SENSITIVITY,  unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_SHARPNESS,          unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_STATIC_THRESHOLD,   unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_TOKEN_PARTITIONS,   int) /* vp8e_token_partitions */

VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_MAXFRAMES,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_STRENGTH,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_ARNR_TYPE,     unsigned int)
VPX_CTRL_USE_TYPE(VP8E_SET_TUNING,             int) /* vp8e_tuning */
VPX_CTRL_USE_TYPE(VP8E_SET_CQ_LEVEL,      unsigned int)

VPX_CTRL_USE_TYPE(VP8E_GET_LAST_QUANTIZER,     int *)
VPX_CTRL_USE_TYPE(VP8E_GET_LAST_QUANTIZER_64,  int *)

VPX_CTRL_USE_TYPE(VP8E_SET_MAX_INTRA_BITRATE_PCT, unsigned int)

VPX_CTRL_USE_TYPE(VP9E_SET_LOSSLESS, unsigned int)

/*! @} - end defgroup vp8_encoder */
#include "vpx_codec_impl_bottom.h"
#endif
