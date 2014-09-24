vp9_common_forward_decls() {
cat <<EOF
/*
 * VP9
 */

#include "vpx/vpx_integer.h"

struct loop_filter_info;
struct blockd;
struct macroblockd;
struct loop_filter_info;

/* Encoder forward decls */
struct block;
struct macroblock;
struct variance_vtable;

#define DEC_MVCOSTS int *mvjcost, int *mvcost[2]
union int_mv;
struct yv12_buffer_config;
EOF
}
forward_decls vp9_common_forward_decls

prototype void vp9_filter_block2d_4x4_8 "const uint8_t *src_ptr, const unsigned int src_stride, const int16_t *HFilter_aligned16, const int16_t *VFilter_aligned16, uint8_t *dst_ptr, unsigned int dst_stride"
prototype void vp9_filter_block2d_8x4_8 "const uint8_t *src_ptr, const unsigned int src_stride, const int16_t *HFilter_aligned16, const int16_t *VFilter_aligned16, uint8_t *dst_ptr, unsigned int dst_stride"
prototype void vp9_filter_block2d_8x8_8 "const uint8_t *src_ptr, const unsigned int src_stride, const int16_t *HFilter_aligned16, const int16_t *VFilter_aligned16, uint8_t *dst_ptr, unsigned int dst_stride"
prototype void vp9_filter_block2d_16x16_8 "const uint8_t *src_ptr, const unsigned int src_stride, const int16_t *HFilter_aligned16, const int16_t *VFilter_aligned16, uint8_t *dst_ptr, unsigned int dst_stride"

# At the very least, MSVC 2008 has compiler bug exhibited by this code; code
# compiles warning free but a dissassembly of generated code show bugs. To be
# on the safe side, only enabled when compiled with 'gcc'.
if [ "$CONFIG_GCC" = "yes" ]; then
    specialize vp9_filter_block2d_4x4_8 sse4_1 sse2
fi
    specialize vp9_filter_block2d_8x4_8 ssse3 #sse4_1 sse2
    specialize vp9_filter_block2d_8x8_8 ssse3 #sse4_1 sse2
    specialize vp9_filter_block2d_16x16_8 ssse3 #sse4_1 sse2

#
# Dequant
#
prototype void vp9_dequantize_b "struct blockd *x"
specialize vp9_dequantize_b

prototype void vp9_dequantize_b_2x2 "struct blockd *x"
specialize vp9_dequantize_b_2x2

prototype void vp9_dequant_dc_idct_add_y_block_8x8 "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dst, int stride, uint16_t *eobs, const int16_t *dc, struct macroblockd *xd"
specialize vp9_dequant_dc_idct_add_y_block_8x8

prototype void vp9_dequant_idct_add_y_block_8x8 "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dst, int stride, uint16_t *eobs, struct macroblockd *xd"
specialize vp9_dequant_idct_add_y_block_8x8

prototype void vp9_dequant_idct_add_uv_block_8x8 "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dstu, uint8_t *dstv, int stride, uint16_t *eobs, struct macroblockd *xd"
specialize vp9_dequant_idct_add_uv_block_8x8

prototype void vp9_dequant_idct_add_16x16 "int16_t *input, const int16_t *dq, uint8_t *pred, uint8_t *dest, int pitch, int stride, int eob"
specialize vp9_dequant_idct_add_16x16

prototype void vp9_dequant_idct_add_8x8 "int16_t *input, const int16_t *dq, uint8_t *pred, uint8_t *dest, int pitch, int stride, int dc, int eob"
specialize vp9_dequant_idct_add_8x8

prototype void vp9_dequant_idct_add "int16_t *input, const int16_t *dq, uint8_t *pred, uint8_t *dest, int pitch, int stride"
specialize vp9_dequant_idct_add

prototype void vp9_dequant_dc_idct_add "int16_t *input, const int16_t *dq, uint8_t *pred, uint8_t *dest, int pitch, int stride, int dc"
specialize vp9_dequant_dc_idct_add

prototype void vp9_dequant_dc_idct_add_y_block "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dst, int stride, uint16_t *eobs, const int16_t *dcs"
specialize vp9_dequant_dc_idct_add_y_block

prototype void vp9_dequant_idct_add_y_block "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dst, int stride, uint16_t *eobs"
specialize vp9_dequant_idct_add_y_block

prototype void vp9_dequant_idct_add_uv_block "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dstu, uint8_t *dstv, int stride, uint16_t *eobs"
specialize vp9_dequant_idct_add_uv_block

prototype void vp9_dequant_idct_add_32x32 "int16_t *q, const int16_t *dq, uint8_t *pre, uint8_t *dst, int pitch, int stride, int eob"
specialize vp9_dequant_idct_add_32x32

prototype void vp9_dequant_idct_add_uv_block_16x16 "int16_t *q, const int16_t *dq, uint8_t *dstu, uint8_t *dstv, int stride, uint16_t *eobs"
specialize vp9_dequant_idct_add_uv_block_16x16

#
# RECON
#
prototype void vp9_copy_mem16x16 "uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch"
specialize vp9_copy_mem16x16 mmx sse2 dspr2
vp9_copy_mem16x16_dspr2=vp9_copy_mem16x16_dspr2

prototype void vp9_copy_mem8x8 "uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch"
specialize vp9_copy_mem8x8 mmx dspr2
vp9_copy_mem8x8_dspr2=vp9_copy_mem8x8_dspr2

prototype void vp9_copy_mem8x4 "uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch"
specialize vp9_copy_mem8x4 mmx

prototype void vp9_avg_mem16x16 "uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch"
specialize vp9_avg_mem16x16

prototype void vp9_avg_mem8x8 "uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch"
specialize vp9_avg_mem8x8

prototype void vp9_copy_mem8x4 "uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch"
specialize vp9_copy_mem8x4 mmx dspr2
vp9_copy_mem8x4_dspr2=vp9_copy_mem8x4_dspr2

prototype void vp9_recon_b "uint8_t *pred_ptr, int16_t *diff_ptr, uint8_t *dst_ptr, int stride"
specialize vp9_recon_b

prototype void vp9_recon_uv_b "uint8_t *pred_ptr, int16_t *diff_ptr, uint8_t *dst_ptr, int stride"
specialize vp9_recon_uv_b

prototype void vp9_recon2b "uint8_t *pred_ptr, int16_t *diff_ptr, uint8_t *dst_ptr, int stride"
specialize vp9_recon2b sse2

prototype void vp9_recon4b "uint8_t *pred_ptr, int16_t *diff_ptr, uint8_t *dst_ptr, int stride"
specialize vp9_recon4b sse2

prototype void vp9_recon_mb "struct macroblockd *x"
specialize vp9_recon_mb

prototype void vp9_recon_mby "struct macroblockd *x"
specialize vp9_recon_mby

prototype void vp9_recon_mby_s "struct macroblockd *x, uint8_t *dst"
specialize vp9_recon_mby_s

prototype void vp9_recon_mbuv_s "struct macroblockd *x, uint8_t *udst, uint8_t *vdst"
specialize void vp9_recon_mbuv_s

prototype void vp9_recon_sby_s "struct macroblockd *x, uint8_t *dst"
specialize vp9_recon_sby_s

prototype void vp9_recon_sbuv_s "struct macroblockd *x, uint8_t *udst, uint8_t *vdst"
specialize void vp9_recon_sbuv_s

prototype void vp9_build_intra_predictors_mby_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_mby_s

prototype void vp9_build_intra_predictors_sby_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_sby_s;

prototype void vp9_build_intra_predictors_sbuv_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_sbuv_s;

prototype void vp9_build_intra_predictors_mby "struct macroblockd *x"
specialize vp9_build_intra_predictors_mby;

prototype void vp9_build_intra_predictors_mby_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_mby_s;

prototype void vp9_build_intra_predictors_mbuv "struct macroblockd *x"
specialize vp9_build_intra_predictors_mbuv;

prototype void vp9_build_intra_predictors_mbuv_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_mbuv_s;

prototype void vp9_build_intra_predictors_sb64y_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_sb64y_s;

prototype void vp9_build_intra_predictors_sb64uv_s "struct macroblockd *x"
specialize vp9_build_intra_predictors_sb64uv_s;

prototype void vp9_intra4x4_predict "struct blockd *x, int b_mode, uint8_t *predictor"
specialize vp9_intra4x4_predict;

prototype void vp9_intra8x8_predict "struct blockd *x, int b_mode, uint8_t *predictor"
specialize vp9_intra8x8_predict;

prototype void vp9_intra_uv4x4_predict "struct blockd *x, int b_mode, uint8_t *predictor"
specialize vp9_intra_uv4x4_predict;

#
# Loopfilter
#
prototype void vp9_loop_filter_mbv "uint8_t *y, uint8_t *u, uint8_t *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_loop_filter_mbv sse2

prototype void vp9_loop_filter_bv "uint8_t *y, uint8_t *u, uint8_t *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_loop_filter_bv sse2

prototype void vp9_loop_filter_bv8x8 "uint8_t *y, uint8_t *u, uint8_t *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_loop_filter_bv8x8 sse2

prototype void vp9_loop_filter_mbh "uint8_t *y, uint8_t *u, uint8_t *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_loop_filter_mbh sse2

prototype void vp9_loop_filter_bh "uint8_t *y, uint8_t *u, uint8_t *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_loop_filter_bh sse2

prototype void vp9_loop_filter_bh8x8 "uint8_t *y, uint8_t *u, uint8_t *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_loop_filter_bh8x8 sse2

prototype void vp9_loop_filter_simple_mbv "uint8_t *y, int ystride, const uint8_t *blimit"
specialize vp9_loop_filter_simple_mbv mmx sse2
vp9_loop_filter_simple_mbv_c=vp9_loop_filter_simple_vertical_edge_c
vp9_loop_filter_simple_mbv_mmx=vp9_loop_filter_simple_vertical_edge_mmx
vp9_loop_filter_simple_mbv_sse2=vp9_loop_filter_simple_vertical_edge_sse2

prototype void vp9_loop_filter_simple_mbh "uint8_t *y, int ystride, const uint8_t *blimit"
specialize vp9_loop_filter_simple_mbh mmx sse2
vp9_loop_filter_simple_mbh_c=vp9_loop_filter_simple_horizontal_edge_c
vp9_loop_filter_simple_mbh_mmx=vp9_loop_filter_simple_horizontal_edge_mmx
vp9_loop_filter_simple_mbh_sse2=vp9_loop_filter_simple_horizontal_edge_sse2

prototype void vp9_loop_filter_simple_bv "uint8_t *y, int ystride, const uint8_t *blimit"
specialize vp9_loop_filter_simple_bv mmx sse2
vp9_loop_filter_simple_bv_c=vp9_loop_filter_bvs_c
vp9_loop_filter_simple_bv_mmx=vp9_loop_filter_bvs_mmx
vp9_loop_filter_simple_bv_sse2=vp9_loop_filter_bvs_sse2

prototype void vp9_loop_filter_simple_bh "uint8_t *y, int ystride, const uint8_t *blimit"
specialize vp9_loop_filter_simple_bh mmx sse2
vp9_loop_filter_simple_bh_c=vp9_loop_filter_bhs_c
vp9_loop_filter_simple_bh_mmx=vp9_loop_filter_bhs_mmx
vp9_loop_filter_simple_bh_sse2=vp9_loop_filter_bhs_sse2

prototype void vp9_lpf_mbh_w "unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr, int y_stride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_lpf_mbh_w sse2

prototype void vp9_lpf_mbv_w "unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr, int y_stride, int uv_stride, struct loop_filter_info *lfi"
specialize vp9_lpf_mbv_w sse2

#
# post proc
#
if [ "$CONFIG_POSTPROC" = "yes" ]; then
prototype void vp9_mbpost_proc_down "uint8_t *dst, int pitch, int rows, int cols, int flimit"
specialize vp9_mbpost_proc_down mmx sse2
vp9_mbpost_proc_down_sse2=vp9_mbpost_proc_down_xmm

prototype void vp9_mbpost_proc_across_ip "uint8_t *src, int pitch, int rows, int cols, int flimit"
specialize vp9_mbpost_proc_across_ip sse2
vp9_mbpost_proc_across_ip_sse2=vp9_mbpost_proc_across_ip_xmm

prototype void vp9_post_proc_down_and_across "uint8_t *src_ptr, uint8_t *dst_ptr, int src_pixels_per_line, int dst_pixels_per_line, int rows, int cols, int flimit"
specialize vp9_post_proc_down_and_across mmx sse2
vp9_post_proc_down_and_across_sse2=vp9_post_proc_down_and_across_xmm

prototype void vp9_plane_add_noise "uint8_t *Start, char *noise, char blackclamp[16], char whiteclamp[16], char bothclamp[16], unsigned int Width, unsigned int Height, int Pitch"
specialize vp9_plane_add_noise mmx sse2
vp9_plane_add_noise_sse2=vp9_plane_add_noise_wmt
fi

prototype void vp9_blend_mb_inner "uint8_t *y, uint8_t *u, uint8_t *v, int y1, int u1, int v1, int alpha, int stride"
specialize vp9_blend_mb_inner

prototype void vp9_blend_mb_outer "uint8_t *y, uint8_t *u, uint8_t *v, int y1, int u1, int v1, int alpha, int stride"
specialize vp9_blend_mb_outer

prototype void vp9_blend_b "uint8_t *y, uint8_t *u, uint8_t *v, int y1, int u1, int v1, int alpha, int stride"
specialize vp9_blend_b

#
# sad 16x3, 3x16
#
prototype unsigned int vp9_sad16x3 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int ref_stride"
specialize vp9_sad16x3 sse2

prototype unsigned int vp9_sad3x16 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int ref_stride"
specialize vp9_sad3x16 sse2

prototype unsigned int vp9_sub_pixel_variance16x2 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance16x2 sse2

#
# Sub Pixel Filters
#
prototype void vp9_eighttap_predict16x16 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict16x16

prototype void vp9_eighttap_predict8x8 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict8x8

prototype void vp9_eighttap_predict_avg16x16 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg16x16

prototype void vp9_eighttap_predict_avg8x8 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg8x8

prototype void vp9_eighttap_predict_avg4x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg4x4

prototype void vp9_eighttap_predict8x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict8x4

prototype void vp9_eighttap_predict4x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict4x4

prototype void vp9_eighttap_predict16x16_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict16x16_sharp

prototype void vp9_eighttap_predict8x8_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict8x8_sharp

prototype void vp9_eighttap_predict_avg16x16_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg16x16_sharp

prototype void vp9_eighttap_predict_avg8x8_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg8x8_sharp

prototype void vp9_eighttap_predict_avg4x4_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg4x4_sharp

prototype void vp9_eighttap_predict8x4_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict8x4_sharp

prototype void vp9_eighttap_predict4x4_sharp "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict4x4_sharp

prototype void vp9_eighttap_predict16x16_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict16x16_smooth

prototype void vp9_eighttap_predict8x8_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict8x8_smooth

prototype void vp9_eighttap_predict_avg16x16_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg16x16_smooth

prototype void vp9_eighttap_predict_avg8x8_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg8x8_smooth

prototype void vp9_eighttap_predict_avg4x4_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict_avg4x4_smooth

prototype void vp9_eighttap_predict8x4_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict8x4_smooth

prototype void vp9_eighttap_predict4x4_smooth "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_eighttap_predict4x4_smooth

prototype void vp9_sixtap_predict16x16 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict16x16

prototype void vp9_sixtap_predict8x8 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict8x8

prototype void vp9_sixtap_predict_avg16x16 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict_avg16x16

prototype void vp9_sixtap_predict_avg8x8 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict_avg8x8

prototype void vp9_sixtap_predict8x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict8x4

prototype void vp9_sixtap_predict4x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict4x4

prototype void vp9_sixtap_predict_avg4x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_sixtap_predict_avg4x4

prototype void vp9_bilinear_predict16x16 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict16x16 sse2

prototype void vp9_bilinear_predict8x8 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict8x8 sse2

prototype void vp9_bilinear_predict_avg16x16 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict_avg16x16

prototype void vp9_bilinear_predict_avg8x8 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict_avg8x8

prototype void vp9_bilinear_predict8x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict8x4

prototype void vp9_bilinear_predict4x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict4x4

prototype void vp9_bilinear_predict_avg4x4 "uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, uint8_t *dst_ptr, int  dst_pitch"
specialize vp9_bilinear_predict_avg4x4

#
# dct
#
prototype void vp9_short_idct4x4llm_1 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct4x4llm_1

prototype void vp9_short_idct4x4llm "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct4x4llm

prototype void vp9_short_idct8x8 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct8x8

prototype void vp9_short_idct10_8x8 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct10_8x8

prototype void vp9_short_ihaar2x2 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_ihaar2x2

prototype void vp9_short_idct16x16 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct16x16

prototype void vp9_short_idct10_16x16 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct10_16x16

prototype void vp9_short_idct32x32 "int16_t *input, int16_t *output, int pitch"
specialize vp9_short_idct32x32

prototype void vp9_ihtllm "const int16_t *input, int16_t *output, int pitch, int tx_type, int tx_dim, int16_t eobs"
specialize vp9_ihtllm

#
# 2nd order
#
prototype void vp9_short_inv_walsh4x4_1 "int16_t *in, int16_t *out"
specialize vp9_short_inv_walsh4x4_1

prototype void vp9_short_inv_walsh4x4 "int16_t *in, int16_t *out"
specialize vp9_short_inv_walsh4x4_


# dct and add
prototype void vp9_dc_only_idct_add_8x8 "int input_dc, uint8_t *pred_ptr, uint8_t *dst_ptr, int pitch, int stride"
specialize vp9_dc_only_idct_add_8x8

prototype void vp9_dc_only_idct_add "int input_dc, uint8_t *pred_ptr, uint8_t *dst_ptr, int pitch, int stride"
specialize vp9_dc_only_idct_add

if [ "$CONFIG_LOSSLESS" = "yes" ]; then
prototype void vp9_short_inv_walsh4x4_1_x8 "int16_t *input, int16_t *output, int pitch"
prototype void vp9_short_inv_walsh4x4_x8 "int16_t *input, int16_t *output, int pitch"
prototype void vp9_dc_only_inv_walsh_add "int input_dc, uint8_t *pred_ptr, uint8_t *dst_ptr, int pitch, int stride"
prototype void vp9_short_inv_walsh4x4_1_lossless "int16_t *in, int16_t *out"
prototype void vp9_short_inv_walsh4x4_lossless "int16_t *in, int16_t *out"
fi

prototype unsigned int vp9_sad32x3 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int ref_stride, int max_sad"
specialize vp9_sad32x3

prototype unsigned int vp9_sad3x32 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int ref_stride, int max_sad"
specialize vp9_sad3x32

#
# Encoder functions below this point.
#
if [ "$CONFIG_VP9_ENCODER" = "yes" ]; then


# variance
[ $arch = "x86_64" ] && mmx_x86_64=mmx && sse2_x86_64=sse2

prototype unsigned int vp9_variance32x32 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance32x32

prototype unsigned int vp9_variance64x64 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance64x64

prototype unsigned int vp9_variance16x16 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance16x16 mmx sse2
vp9_variance16x16_sse2=vp9_variance16x16_wmt
vp9_variance16x16_mmx=vp9_variance16x16_mmx

prototype unsigned int vp9_variance16x8 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance16x8 mmx sse2
vp9_variance16x8_sse2=vp9_variance16x8_wmt
vp9_variance16x8_mmx=vp9_variance16x8_mmx

prototype unsigned int vp9_variance8x16 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance8x16 mmx sse2
vp9_variance8x16_sse2=vp9_variance8x16_wmt
vp9_variance8x16_mmx=vp9_variance8x16_mmx

prototype unsigned int vp9_variance8x8 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance8x8 mmx sse2
vp9_variance8x8_sse2=vp9_variance8x8_wmt
vp9_variance8x8_mmx=vp9_variance8x8_mmx

prototype unsigned int vp9_variance4x4 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance4x4 mmx sse2
vp9_variance4x4_sse2=vp9_variance4x4_wmt
vp9_variance4x4_mmx=vp9_variance4x4_mmx

prototype unsigned int vp9_sub_pixel_variance64x64 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance64x64

prototype unsigned int vp9_sub_pixel_variance32x32 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance32x32

prototype unsigned int vp9_sub_pixel_variance16x16 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance16x16 sse2 mmx ssse3
vp9_sub_pixel_variance16x16_sse2=vp9_sub_pixel_variance16x16_wmt

prototype unsigned int vp9_sub_pixel_variance8x16 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance8x16 sse2 mmx
vp9_sub_pixel_variance8x16_sse2=vp9_sub_pixel_variance8x16_wmt

prototype unsigned int vp9_sub_pixel_variance16x8 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance16x8 sse2 mmx ssse3
vp9_sub_pixel_variance16x8_sse2=vp9_sub_pixel_variance16x8_ssse3;
vp9_sub_pixel_variance16x8_sse2=vp9_sub_pixel_variance16x8_wmt

prototype unsigned int vp9_sub_pixel_variance8x8 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance8x8 sse2 mmx
vp9_sub_pixel_variance8x8_sse2=vp9_sub_pixel_variance8x8_wmt

prototype unsigned int vp9_sub_pixel_variance4x4 "const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_variance4x4 sse2 mmx
vp9_sub_pixel_variance4x4_sse2=vp9_sub_pixel_variance4x4_wmt

prototype unsigned int vp9_sad64x64 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad64x64

prototype unsigned int vp9_sad32x32 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad32x32

prototype unsigned int vp9_sad16x16 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad16x16 mmx sse2 sse3
vp9_sad16x16_sse2=vp9_sad16x16_wmt

prototype unsigned int vp9_sad16x8 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad16x8 mmx sse2
vp9_sad16x8_sse2=vp9_sad16x8_wmt

prototype unsigned int vp9_sad8x16 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad8x16 mmx sse2
vp9_sad8x16_sse2=vp9_sad8x16_wmt

prototype unsigned int vp9_sad8x8 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad8x8 mmx sse2
vp9_sad8x8_sse2=vp9_sad8x8_wmt

prototype unsigned int vp9_sad4x4 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int max_sad"
specialize vp9_sad4x4 mmx sse2
vp9_sad4x4_sse2=vp9_sad4x4_wmt

prototype unsigned int vp9_variance_halfpixvar16x16_h "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar16x16_h mmx sse2
vp9_variance_halfpixvar16x16_h_sse2=vp9_variance_halfpixvar16x16_h_wmt

prototype unsigned int vp9_variance_halfpixvar16x16_v "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar16x16_v mmx sse2
vp9_variance_halfpixvar16x16_v_sse2=vp9_variance_halfpixvar16x16_v_wmt

prototype unsigned int vp9_variance_halfpixvar16x16_hv "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar16x16_hv mmx sse2
vp9_variance_halfpixvar16x16_hv_sse2=vp9_variance_halfpixvar16x16_hv_wmt

prototype unsigned int vp9_variance_halfpixvar64x64_h "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar64x64_h

prototype unsigned int vp9_variance_halfpixvar64x64_v "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar64x64_v

prototype unsigned int vp9_variance_halfpixvar64x64_hv "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar64x64_hv

prototype unsigned int vp9_variance_halfpixvar32x32_h "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar32x32_h

prototype unsigned int vp9_variance_halfpixvar32x32_v "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar32x32_v

prototype unsigned int vp9_variance_halfpixvar32x32_hv "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse"
specialize vp9_variance_halfpixvar32x32_hv

prototype void vp9_sad64x64x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad64x64x3

prototype void vp9_sad32x32x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad32x32x3

prototype void vp9_sad16x16x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad16x16x3 sse3 ssse3

prototype void vp9_sad16x8x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad16x8x3 sse3 ssse3

prototype void vp9_sad8x16x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad8x16x3 sse3

prototype void vp9_sad8x8x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad8x8x3 sse3

prototype void vp9_sad4x4x3 "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad4x4x3 sse3

prototype void vp9_sad64x64x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad64x64x8

prototype void vp9_sad32x32x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad32x32x8

prototype void vp9_sad16x16x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad16x16x8 sse4

prototype void vp9_sad16x8x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad16x8x8 sse4

prototype void vp9_sad8x16x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad8x16x8 sse4

prototype void vp9_sad8x8x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad8x8x8 sse4

prototype void vp9_sad4x4x8 "const uint8_t *src_ptr, int  src_stride, const uint8_t *ref_ptr, int  ref_stride, uint16_t *sad_array"
specialize vp9_sad4x4x8 sse4

prototype void vp9_sad64x64x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad64x64x4d

prototype void vp9_sad32x32x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad32x32x4d

prototype void vp9_sad16x16x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad16x16x4d sse3

prototype void vp9_sad16x8x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad16x8x4d sse3

prototype void vp9_sad8x16x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad8x16x4d sse3

prototype void vp9_sad8x8x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad8x8x4d sse3

prototype void vp9_sad4x4x4d "const uint8_t *src_ptr, int  src_stride, const uint8_t **ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp9_sad4x4x4d sse3

#
# Block copy
#
case $arch in
    x86*)
    prototype void vp9_copy32xn "const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, int n"
    specialize vp9_copy32xn sse2 sse3
    ;;
esac

prototype unsigned int vp9_sub_pixel_mse16x16 "const uint8_t *src_ptr, int  src_pixels_per_line, int  xoffset, int  yoffset, const uint8_t *dst_ptr, int dst_pixels_per_line, unsigned int *sse"
specialize vp9_sub_pixel_mse16x16 sse2 mmx
vp9_sub_pixel_mse16x16_sse2=vp9_sub_pixel_mse16x16_wmt

prototype unsigned int vp9_mse16x16 "const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse"
specialize vp9_mse16x16 mmx sse2
vp9_mse16x16_sse2=vp9_mse16x16_wmt

prototype unsigned int vp9_sub_pixel_mse64x64 "const uint8_t *src_ptr, int  source_stride, int  xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_mse64x64

prototype unsigned int vp9_sub_pixel_mse32x32 "const uint8_t *src_ptr, int  source_stride, int  xoffset, int  yoffset, const uint8_t *ref_ptr, int Refstride, unsigned int *sse"
specialize vp9_sub_pixel_mse32x32

prototype unsigned int vp9_get_mb_ss "const int16_t *"
specialize vp9_get_mb_ss mmx sse2
# ENCODEMB INVOKE
prototype int vp9_mbblock_error "struct macroblock *mb, int dc"
specialize vp9_mbblock_error mmx sse2
vp9_mbblock_error_sse2=vp9_mbblock_error_xmm

prototype int vp9_block_error "int16_t *coeff, int16_t *dqcoeff, int block_size"
specialize vp9_block_error mmx sse2
vp9_block_error_sse2=vp9_block_error_xmm

prototype void vp9_subtract_b "struct block *be, struct blockd *bd, int pitch"
specialize vp9_subtract_b mmx sse2

prototype int vp9_mbuverror "struct macroblock *mb"
specialize vp9_mbuverror mmx sse2
vp9_mbuverror_sse2=vp9_mbuverror_xmm

prototype void vp9_subtract_b "struct block *be, struct blockd *bd, int pitch"
specialize vp9_subtract_b mmx sse2

prototype void vp9_subtract_mby "int16_t *diff, uint8_t *src, uint8_t *pred, int stride"
specialize vp9_subtract_mby mmx sse2

prototype void vp9_subtract_mbuv "int16_t *diff, uint8_t *usrc, uint8_t *vsrc, uint8_t *pred, int stride"
specialize vp9_subtract_mbuv mmx sse2

#
# Structured Similarity (SSIM)
#
if [ "$CONFIG_INTERNAL_STATS" = "yes" ]; then
    [ $arch = "x86_64" ] && sse2_on_x86_64=sse2

    prototype void vp9_ssim_parms_8x8 "uint8_t *s, int sp, uint8_t *r, int rp, unsigned long *sum_s, unsigned long *sum_r, unsigned long *sum_sq_s, unsigned long *sum_sq_r, unsigned long *sum_sxr"
    specialize vp9_ssim_parms_8x8 $sse2_on_x86_64

    prototype void vp9_ssim_parms_16x16 "uint8_t *s, int sp, uint8_t *r, int rp, unsigned long *sum_s, unsigned long *sum_r, unsigned long *sum_sq_s, unsigned long *sum_sq_r, unsigned long *sum_sxr"
    specialize vp9_ssim_parms_16x16 $sse2_on_x86_64
fi

# fdct functions
prototype void vp9_fht "const int16_t *input, int pitch, int16_t *output, int tx_type, int tx_dim"
specialize vp9_fht

prototype void vp9_short_fdct8x8 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_fdct8x8

prototype void vp9_short_fhaar2x2 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_fhaar2x2

prototype void vp9_short_fdct4x4 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_fdct4x4

prototype void vp9_short_fdct8x4 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_fdct8x4

prototype void vp9_short_walsh4x4 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_walsh4x4

prototype void vp9_short_fdct32x32 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_fdct32x32

prototype void vp9_short_fdct16x16 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_fdct16x16

prototype void vp9_short_walsh4x4_lossless "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_walsh4x4_lossless

prototype void vp9_short_walsh4x4_x8 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_walsh4x4_x8

prototype void vp9_short_walsh8x4_x8 "int16_t *InputData, int16_t *OutputData, int pitch"
specialize vp9_short_walsh8x4_x8

#
# Motion search
#
prototype int vp9_full_search_sad "struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, int sad_per_bit, int distance, struct variance_vtable *fn_ptr, DEC_MVCOSTS, union int_mv *center_mv"
specialize vp9_full_search_sad sse3 sse4_1
vp9_full_search_sad_sse3=vp9_full_search_sadx3
vp9_full_search_sad_sse4_1=vp9_full_search_sadx8

prototype int vp9_refining_search_sad "struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, int sad_per_bit, int distance, struct variance_vtable *fn_ptr, DEC_MVCOSTS, union int_mv *center_mv"
specialize vp9_refining_search_sad sse3
vp9_refining_search_sad_sse3=vp9_refining_search_sadx4

prototype int vp9_diamond_search_sad "struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, union int_mv *best_mv, int search_param, int sad_per_bit, int *num00, struct variance_vtable *fn_ptr, DEC_MVCOSTS, union int_mv *center_mv"
specialize vp9_diamond_search_sad sse3
vp9_diamond_search_sad_sse3=vp9_diamond_search_sadx4

prototype void vp9_temporal_filter_apply "uint8_t *frame1, unsigned int stride, uint8_t *frame2, unsigned int block_size, int strength, int filter_weight, unsigned int *accumulator, uint16_t *count"
specialize vp9_temporal_filter_apply sse2

prototype void vp9_yv12_copy_partial_frame "struct yv12_buffer_config *src_ybc, struct yv12_buffer_config *dst_ybc, int fraction"
specialize vp9_yv12_copy_partial_frame


fi
# end encoder functions
