vp8_common_forward_decls() {
cat <<EOF
/*
 * VP8
 */

struct blockd;
struct macroblockd;
struct loop_filter_info;

/* Encoder forward decls */
struct block;
struct macroblock;
struct variance_vtable;
union int_mv;
struct yv12_buffer_config;
EOF
}
forward_decls vp8_common_forward_decls

#
# system state
#
prototype void vp8_clear_system_state ""
specialize vp8_clear_system_state mmx
vp8_clear_system_state_mmx=vpx_reset_mmx_state

#
# Dequant
#
prototype void vp8_dequantize_b "struct blockd*, short *dqc"
specialize vp8_dequantize_b mmx media neon
vp8_dequantize_b_media=vp8_dequantize_b_v6

prototype void vp8_dequant_idct_add "short *input, short *dq, unsigned char *output, int stride"
specialize vp8_dequant_idct_add mmx media neon dspr2
vp8_dequant_idct_add_media=vp8_dequant_idct_add_v6
vp8_dequant_idct_add_dspr2=vp8_dequant_idct_add_dspr2

prototype void vp8_dequant_idct_add_y_block "short *q, short *dq, unsigned char *dst, int stride, char *eobs"
specialize vp8_dequant_idct_add_y_block mmx sse2 media neon dspr2
vp8_dequant_idct_add_y_block_media=vp8_dequant_idct_add_y_block_v6
vp8_dequant_idct_add_y_block_dspr2=vp8_dequant_idct_add_y_block_dspr2

prototype void vp8_dequant_idct_add_uv_block "short *q, short *dq, unsigned char *dst_u, unsigned char *dst_v, int stride, char *eobs"
specialize vp8_dequant_idct_add_uv_block mmx sse2 media neon dspr2
vp8_dequant_idct_add_uv_block_media=vp8_dequant_idct_add_uv_block_v6
vp8_dequant_idct_add_y_block_dspr2=vp8_dequant_idct_add_y_block_dspr2

#
# Loopfilter
#
prototype void vp8_loop_filter_mbv "unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp8_loop_filter_mbv mmx sse2 media neon dspr2
vp8_loop_filter_mbv_media=vp8_loop_filter_mbv_armv6
vp8_loop_filter_mbv_dspr2=vp8_loop_filter_mbv_dspr2

prototype void vp8_loop_filter_bv "unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp8_loop_filter_bv mmx sse2 media neon dspr2
vp8_loop_filter_bv_media=vp8_loop_filter_bv_armv6
vp8_loop_filter_bv_dspr2=vp8_loop_filter_bv_dspr2

prototype void vp8_loop_filter_mbh "unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp8_loop_filter_mbh mmx sse2 media neon dspr2
vp8_loop_filter_mbh_media=vp8_loop_filter_mbh_armv6
vp8_loop_filter_mbh_dspr2=vp8_loop_filter_mbh_dspr2

prototype void vp8_loop_filter_bh "unsigned char *y, unsigned char *u, unsigned char *v, int ystride, int uv_stride, struct loop_filter_info *lfi"
specialize vp8_loop_filter_bh mmx sse2 media neon dspr2
vp8_loop_filter_bh_media=vp8_loop_filter_bh_armv6
vp8_loop_filter_bh_dspr2=vp8_loop_filter_bh_dspr2


prototype void vp8_loop_filter_simple_mbv "unsigned char *y, int ystride, const unsigned char *blimit"
specialize vp8_loop_filter_simple_mbv mmx sse2 media neon
vp8_loop_filter_simple_mbv_c=vp8_loop_filter_simple_vertical_edge_c
vp8_loop_filter_simple_mbv_mmx=vp8_loop_filter_simple_vertical_edge_mmx
vp8_loop_filter_simple_mbv_sse2=vp8_loop_filter_simple_vertical_edge_sse2
vp8_loop_filter_simple_mbv_media=vp8_loop_filter_simple_vertical_edge_armv6
vp8_loop_filter_simple_mbv_neon=vp8_loop_filter_mbvs_neon

prototype void vp8_loop_filter_simple_mbh "unsigned char *y, int ystride, const unsigned char *blimit"
specialize vp8_loop_filter_simple_mbh mmx sse2 media neon
vp8_loop_filter_simple_mbh_c=vp8_loop_filter_simple_horizontal_edge_c
vp8_loop_filter_simple_mbh_mmx=vp8_loop_filter_simple_horizontal_edge_mmx
vp8_loop_filter_simple_mbh_sse2=vp8_loop_filter_simple_horizontal_edge_sse2
vp8_loop_filter_simple_mbh_media=vp8_loop_filter_simple_horizontal_edge_armv6
vp8_loop_filter_simple_mbh_neon=vp8_loop_filter_mbhs_neon

prototype void vp8_loop_filter_simple_bv "unsigned char *y, int ystride, const unsigned char *blimit"
specialize vp8_loop_filter_simple_bv mmx sse2 media neon
vp8_loop_filter_simple_bv_c=vp8_loop_filter_bvs_c
vp8_loop_filter_simple_bv_mmx=vp8_loop_filter_bvs_mmx
vp8_loop_filter_simple_bv_sse2=vp8_loop_filter_bvs_sse2
vp8_loop_filter_simple_bv_media=vp8_loop_filter_bvs_armv6
vp8_loop_filter_simple_bv_neon=vp8_loop_filter_bvs_neon

prototype void vp8_loop_filter_simple_bh "unsigned char *y, int ystride, const unsigned char *blimit"
specialize vp8_loop_filter_simple_bh mmx sse2 media neon
vp8_loop_filter_simple_bh_c=vp8_loop_filter_bhs_c
vp8_loop_filter_simple_bh_mmx=vp8_loop_filter_bhs_mmx
vp8_loop_filter_simple_bh_sse2=vp8_loop_filter_bhs_sse2
vp8_loop_filter_simple_bh_media=vp8_loop_filter_bhs_armv6
vp8_loop_filter_simple_bh_neon=vp8_loop_filter_bhs_neon

#
# IDCT
#
#idct16
prototype void vp8_short_idct4x4llm "short *input, unsigned char *pred, int pitch, unsigned char *dst, int dst_stride"
specialize vp8_short_idct4x4llm mmx media neon dspr2
vp8_short_idct4x4llm_media=vp8_short_idct4x4llm_v6_dual
vp8_short_idct4x4llm_dspr2=vp8_short_idct4x4llm_dspr2

#iwalsh1
prototype void vp8_short_inv_walsh4x4_1 "short *input, short *output"
specialize vp8_short_inv_walsh4x4_1 dspr2
vp8_short_inv_walsh4x4_1_dspr2=vp8_short_inv_walsh4x4_1_dspr2
# no asm yet

#iwalsh16
prototype void vp8_short_inv_walsh4x4 "short *input, short *output"
specialize vp8_short_inv_walsh4x4 mmx sse2 media neon dspr2
vp8_short_inv_walsh4x4_media=vp8_short_inv_walsh4x4_v6
vp8_short_inv_walsh4x4_dspr2=vp8_short_inv_walsh4x4_dspr2

#idct1_scalar_add
prototype void vp8_dc_only_idct_add "short input, unsigned char *pred, int pred_stride, unsigned char *dst, int dst_stride"
specialize vp8_dc_only_idct_add	mmx media neon dspr2
vp8_dc_only_idct_add_media=vp8_dc_only_idct_add_v6
vp8_dc_only_idct_add_dspr2=vp8_dc_only_idct_add_dspr2

#
# RECON
#
prototype void vp8_copy_mem16x16 "unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch"
specialize vp8_copy_mem16x16 mmx sse2 media neon dspr2
vp8_copy_mem16x16_media=vp8_copy_mem16x16_v6
vp8_copy_mem16x16_dspr2=vp8_copy_mem16x16_dspr2

prototype void vp8_copy_mem8x8 "unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch"
specialize vp8_copy_mem8x8 mmx media neon dspr2
vp8_copy_mem8x8_media=vp8_copy_mem8x8_v6
vp8_copy_mem8x8_dspr2=vp8_copy_mem8x8_dspr2

prototype void vp8_copy_mem8x4 "unsigned char *src, int src_pitch, unsigned char *dst, int dst_pitch"
specialize vp8_copy_mem8x4 mmx media neon dspr2
vp8_copy_mem8x4_media=vp8_copy_mem8x4_v6
vp8_copy_mem8x4_dspr2=vp8_copy_mem8x4_dspr2

prototype void vp8_build_intra_predictors_mby_s "struct macroblockd *x, unsigned char * yabove_row, unsigned char * yleft, int left_stride, unsigned char * ypred_ptr, int y_stride"
specialize vp8_build_intra_predictors_mby_s sse2 ssse3
#TODO: fix assembly for neon

prototype void vp8_build_intra_predictors_mbuv_s "struct macroblockd *x, unsigned char * uabove_row, unsigned char * vabove_row,  unsigned char *uleft, unsigned char *vleft, int left_stride, unsigned char * upred_ptr, unsigned char * vpred_ptr, int pred_stride"
specialize vp8_build_intra_predictors_mbuv_s sse2 ssse3

prototype void vp8_intra4x4_predict "unsigned char *Above, unsigned char *yleft, int left_stride, int b_mode, unsigned char *dst, int dst_stride, unsigned char top_left"
specialize vp8_intra4x4_predict media
vp8_intra4x4_predict_media=vp8_intra4x4_predict_armv6

#
# Postproc
#
if [ "$CONFIG_POSTPROC" = "yes" ]; then
    prototype void vp8_mbpost_proc_down "unsigned char *dst, int pitch, int rows, int cols,int flimit"
    specialize vp8_mbpost_proc_down mmx sse2
    vp8_mbpost_proc_down_sse2=vp8_mbpost_proc_down_xmm

    prototype void vp8_mbpost_proc_across_ip "unsigned char *dst, int pitch, int rows, int cols,int flimit"
    specialize vp8_mbpost_proc_across_ip sse2
    vp8_mbpost_proc_across_ip_sse2=vp8_mbpost_proc_across_ip_xmm

    prototype void vp8_post_proc_down_and_across_mb_row "unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, int cols, unsigned char *flimits, int size"
    specialize vp8_post_proc_down_and_across_mb_row sse2

    prototype void vp8_plane_add_noise "unsigned char *s, char *noise, char blackclamp[16], char whiteclamp[16], char bothclamp[16], unsigned int w, unsigned int h, int pitch"
    specialize vp8_plane_add_noise mmx sse2
    vp8_plane_add_noise_sse2=vp8_plane_add_noise_wmt

    prototype void vp8_blend_mb_inner "unsigned char *y, unsigned char *u, unsigned char *v, int y1, int u1, int v1, int alpha, int stride"
    # no asm yet

    prototype void vp8_blend_mb_outer "unsigned char *y, unsigned char *u, unsigned char *v, int y1, int u1, int v1, int alpha, int stride"
    # no asm yet

    prototype void vp8_blend_b "unsigned char *y, unsigned char *u, unsigned char *v, int y1, int u1, int v1, int alpha, int stride"
    # no asm yet

    prototype void vp8_filter_by_weight16x16 "unsigned char *src, int src_stride, unsigned char *dst, int dst_stride, int src_weight"
    specialize vp8_filter_by_weight16x16 sse2

    prototype void vp8_filter_by_weight8x8 "unsigned char *src, int src_stride, unsigned char *dst, int dst_stride, int src_weight"
    specialize vp8_filter_by_weight8x8 sse2

    prototype void vp8_filter_by_weight4x4 "unsigned char *src, int src_stride, unsigned char *dst, int dst_stride, int src_weight"
    # no asm yet
fi

#
# Subpixel
#
prototype void vp8_sixtap_predict16x16 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_sixtap_predict16x16 mmx sse2 ssse3 media neon dspr2
vp8_sixtap_predict16x16_media=vp8_sixtap_predict16x16_armv6
vp8_sixtap_predict16x16_dspr2=vp8_sixtap_predict16x16_dspr2

prototype void vp8_sixtap_predict8x8 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_sixtap_predict8x8 mmx sse2 ssse3 media neon dspr2
vp8_sixtap_predict8x8_media=vp8_sixtap_predict8x8_armv6
vp8_sixtap_predict8x8_dspr2=vp8_sixtap_predict8x8_dspr2

prototype void vp8_sixtap_predict8x4 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_sixtap_predict8x4 mmx sse2 ssse3 media neon dspr2
vp8_sixtap_predict8x4_media=vp8_sixtap_predict8x4_armv6
vp8_sixtap_predict8x4_dspr2=vp8_sixtap_predict8x4_dspr2

prototype void vp8_sixtap_predict4x4 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_sixtap_predict4x4 mmx ssse3 media neon dspr2
vp8_sixtap_predict4x4_media=vp8_sixtap_predict4x4_armv6
vp8_sixtap_predict4x4_dspr2=vp8_sixtap_predict4x4_dspr2

prototype void vp8_bilinear_predict16x16 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_bilinear_predict16x16 mmx sse2 ssse3 media neon
vp8_bilinear_predict16x16_media=vp8_bilinear_predict16x16_armv6

prototype void vp8_bilinear_predict8x8 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_bilinear_predict8x8 mmx sse2 ssse3 media neon
vp8_bilinear_predict8x8_media=vp8_bilinear_predict8x8_armv6

prototype void vp8_bilinear_predict8x4 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_bilinear_predict8x4 mmx media neon
vp8_bilinear_predict8x4_media=vp8_bilinear_predict8x4_armv6

prototype void vp8_bilinear_predict4x4 "unsigned char *src, int src_pitch, int xofst, int yofst, unsigned char *dst, int dst_pitch"
specialize vp8_bilinear_predict4x4 mmx media neon
vp8_bilinear_predict4x4_media=vp8_bilinear_predict4x4_armv6

#
# Whole-pixel Variance
#
prototype unsigned int vp8_variance4x4 "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance4x4 mmx sse2
vp8_variance4x4_sse2=vp8_variance4x4_wmt

prototype unsigned int vp8_variance8x8 "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance8x8 mmx sse2 media neon
vp8_variance8x8_sse2=vp8_variance8x8_wmt
vp8_variance8x8_media=vp8_variance8x8_armv6

prototype unsigned int vp8_variance8x16 "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance8x16 mmx sse2 neon
vp8_variance8x16_sse2=vp8_variance8x16_wmt

prototype unsigned int vp8_variance16x8 "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance16x8 mmx sse2 neon
vp8_variance16x8_sse2=vp8_variance16x8_wmt

prototype unsigned int vp8_variance16x16 "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance16x16 mmx sse2 media neon
vp8_variance16x16_sse2=vp8_variance16x16_wmt
vp8_variance16x16_media=vp8_variance16x16_armv6

#
# Sub-pixel Variance
#
prototype unsigned int vp8_sub_pixel_variance4x4 "const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse"
specialize vp8_sub_pixel_variance4x4 mmx sse2
vp8_sub_pixel_variance4x4_sse2=vp8_sub_pixel_variance4x4_wmt

prototype unsigned int vp8_sub_pixel_variance8x8 "const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse"
specialize vp8_sub_pixel_variance8x8 mmx sse2 media neon
vp8_sub_pixel_variance8x8_sse2=vp8_sub_pixel_variance8x8_wmt
vp8_sub_pixel_variance8x8_media=vp8_sub_pixel_variance8x8_armv6

prototype unsigned int vp8_sub_pixel_variance8x16 "const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse"
specialize vp8_sub_pixel_variance8x16 mmx sse2
vp8_sub_pixel_variance8x16_sse2=vp8_sub_pixel_variance8x16_wmt

prototype unsigned int vp8_sub_pixel_variance16x8 "const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse"
specialize vp8_sub_pixel_variance16x8 mmx sse2 ssse3
vp8_sub_pixel_variance16x8_sse2=vp8_sub_pixel_variance16x8_wmt

prototype unsigned int vp8_sub_pixel_variance16x16 "const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse"
specialize vp8_sub_pixel_variance16x16 mmx sse2 ssse3 media neon
vp8_sub_pixel_variance16x16_sse2=vp8_sub_pixel_variance16x16_wmt
vp8_sub_pixel_variance16x16_media=vp8_sub_pixel_variance16x16_armv6

prototype unsigned int vp8_variance_halfpixvar16x16_h "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance_halfpixvar16x16_h mmx sse2 media neon
vp8_variance_halfpixvar16x16_h_sse2=vp8_variance_halfpixvar16x16_h_wmt
vp8_variance_halfpixvar16x16_h_media=vp8_variance_halfpixvar16x16_h_armv6

prototype unsigned int vp8_variance_halfpixvar16x16_v "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance_halfpixvar16x16_v mmx sse2 media neon
vp8_variance_halfpixvar16x16_v_sse2=vp8_variance_halfpixvar16x16_v_wmt
vp8_variance_halfpixvar16x16_v_media=vp8_variance_halfpixvar16x16_v_armv6

prototype unsigned int vp8_variance_halfpixvar16x16_hv "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_variance_halfpixvar16x16_hv mmx sse2 media neon
vp8_variance_halfpixvar16x16_hv_sse2=vp8_variance_halfpixvar16x16_hv_wmt
vp8_variance_halfpixvar16x16_hv_media=vp8_variance_halfpixvar16x16_hv_armv6

#
# Single block SAD
#
prototype unsigned int vp8_sad4x4 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad"
specialize vp8_sad4x4 mmx sse2 neon
vp8_sad4x4_sse2=vp8_sad4x4_wmt

prototype unsigned int vp8_sad8x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad"
specialize vp8_sad8x8 mmx sse2 neon
vp8_sad8x8_sse2=vp8_sad8x8_wmt

prototype unsigned int vp8_sad8x16 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad"
specialize vp8_sad8x16 mmx sse2 neon
vp8_sad8x16_sse2=vp8_sad8x16_wmt

prototype unsigned int vp8_sad16x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad"
specialize vp8_sad16x8 mmx sse2 neon
vp8_sad16x8_sse2=vp8_sad16x8_wmt

prototype unsigned int vp8_sad16x16 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int ref_stride, unsigned int max_sad"
specialize vp8_sad16x16 mmx sse2 sse3 media neon
vp8_sad16x16_sse2=vp8_sad16x16_wmt
vp8_sad16x16_media=vp8_sad16x16_armv6

#
# Multi-block SAD, comparing a reference to N blocks 1 pixel apart horizontally
#
prototype void vp8_sad4x4x3 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp8_sad4x4x3 sse3

prototype void vp8_sad8x8x3 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp8_sad8x8x3 sse3

prototype void vp8_sad8x16x3 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp8_sad8x16x3 sse3

prototype void vp8_sad16x8x3 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp8_sad16x8x3 sse3 ssse3

prototype void vp8_sad16x16x3 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sad_array"
specialize vp8_sad16x16x3 sse3 ssse3

# Note the only difference in the following prototypes is that they return into
# an array of short
prototype void vp8_sad4x4x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array"
specialize vp8_sad4x4x8 sse4_1
vp8_sad4x4x8_sse4_1=vp8_sad4x4x8_sse4

prototype void vp8_sad8x8x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array"
specialize vp8_sad8x8x8 sse4_1
vp8_sad8x8x8_sse4_1=vp8_sad8x8x8_sse4

prototype void vp8_sad8x16x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array"
specialize vp8_sad8x16x8 sse4_1
vp8_sad8x16x8_sse4_1=vp8_sad8x16x8_sse4

prototype void vp8_sad16x8x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array"
specialize vp8_sad16x8x8 sse4_1
vp8_sad16x8x8_sse4_1=vp8_sad16x8x8_sse4

prototype void vp8_sad16x16x8 "const unsigned char *src_ptr, int src_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned short *sad_array"
specialize vp8_sad16x16x8 sse4_1
vp8_sad16x16x8_sse4_1=vp8_sad16x16x8_sse4

#
# Multi-block SAD, comparing a reference to N independent blocks
#
prototype void vp8_sad4x4x4d "const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array"
specialize vp8_sad4x4x4d sse3

prototype void vp8_sad8x8x4d "const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array"
specialize vp8_sad8x8x4d sse3

prototype void vp8_sad8x16x4d "const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array"
specialize vp8_sad8x16x4d sse3

prototype void vp8_sad16x8x4d "const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array"
specialize vp8_sad16x8x4d sse3

prototype void vp8_sad16x16x4d "const unsigned char *src_ptr, int src_stride, const unsigned char * const ref_ptr[], int  ref_stride, unsigned int *sad_array"
specialize vp8_sad16x16x4d sse3

#
# Encoder functions below this point.
#
if [ "$CONFIG_VP8_ENCODER" = "yes" ]; then

#
# Sum of squares (vector)
#
prototype unsigned int vp8_get_mb_ss "const short *"
specialize vp8_get_mb_ss mmx sse2

#
# SSE (Sum Squared Error)
#
prototype unsigned int vp8_sub_pixel_mse16x16 "const unsigned char  *src_ptr, int  source_stride, int  xoffset, int  yoffset, const unsigned char *ref_ptr, int Refstride, unsigned int *sse"
specialize vp8_sub_pixel_mse16x16 mmx sse2
vp8_sub_pixel_mse16x16_sse2=vp8_sub_pixel_mse16x16_wmt

prototype unsigned int vp8_mse16x16 "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride, unsigned int *sse"
specialize vp8_mse16x16 mmx sse2 media neon
vp8_mse16x16_sse2=vp8_mse16x16_wmt
vp8_mse16x16_media=vp8_mse16x16_armv6

prototype unsigned int vp8_get4x4sse_cs "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride"
specialize vp8_get4x4sse_cs mmx neon

#
# Block copy
#
case $arch in
    x86*)
    prototype void vp8_copy32xn "const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int ref_stride, int n"
    specialize vp8_copy32xn sse2 sse3
    ;;
esac

#
# Structured Similarity (SSIM)
#
if [ "$CONFIG_INTERNAL_STATS" = "yes" ]; then
    [ $arch = "x86_64" ] && sse2_on_x86_64=sse2

    prototype void vp8_ssim_parms_8x8 "unsigned char *s, int sp, unsigned char *r, int rp, unsigned long *sum_s, unsigned long *sum_r, unsigned long *sum_sq_s, unsigned long *sum_sq_r, unsigned long *sum_sxr"
    specialize vp8_ssim_parms_8x8 $sse2_on_x86_64

    prototype void vp8_ssim_parms_16x16 "unsigned char *s, int sp, unsigned char *r, int rp, unsigned long *sum_s, unsigned long *sum_r, unsigned long *sum_sq_s, unsigned long *sum_sq_r, unsigned long *sum_sxr"
    specialize vp8_ssim_parms_16x16 $sse2_on_x86_64
fi

#
# Forward DCT
#
prototype void vp8_short_fdct4x4 "short *input, short *output, int pitch"
specialize vp8_short_fdct4x4 mmx sse2 media neon
vp8_short_fdct4x4_media=vp8_short_fdct4x4_armv6

prototype void vp8_short_fdct8x4 "short *input, short *output, int pitch"
specialize vp8_short_fdct8x4 mmx sse2 media neon
vp8_short_fdct8x4_media=vp8_short_fdct8x4_armv6

prototype void vp8_short_walsh4x4 "short *input, short *output, int pitch"
specialize vp8_short_walsh4x4 sse2 media neon
vp8_short_walsh4x4_media=vp8_short_walsh4x4_armv6

#
# Quantizer
#
prototype void vp8_regular_quantize_b "struct block *, struct blockd *"
specialize vp8_regular_quantize_b sse2 #sse4_1
# TODO(johann) Update sse4 implementation and re-enable
#vp8_regular_quantize_b_sse4_1=vp8_regular_quantize_b_sse4

prototype void vp8_fast_quantize_b "struct block *, struct blockd *"
specialize vp8_fast_quantize_b sse2 ssse3 media neon
vp8_fast_quantize_b_media=vp8_fast_quantize_b_armv6

prototype void vp8_regular_quantize_b_pair "struct block *b1, struct block *b2, struct blockd *d1, struct blockd *d2"
# no asm yet

prototype void vp8_fast_quantize_b_pair "struct block *b1, struct block *b2, struct blockd *d1, struct blockd *d2"
specialize vp8_fast_quantize_b_pair neon

prototype void vp8_quantize_mb "struct macroblock *"
specialize vp8_quantize_mb neon

prototype void vp8_quantize_mby "struct macroblock *"
specialize vp8_quantize_mby neon

prototype void vp8_quantize_mbuv "struct macroblock *"
specialize vp8_quantize_mbuv neon

#
# Block subtraction
#
prototype int vp8_block_error "short *coeff, short *dqcoeff"
specialize vp8_block_error mmx sse2
vp8_block_error_sse2=vp8_block_error_xmm

prototype int vp8_mbblock_error "struct macroblock *mb, int dc"
specialize vp8_mbblock_error mmx sse2
vp8_mbblock_error_sse2=vp8_mbblock_error_xmm

prototype int vp8_mbuverror "struct macroblock *mb"
specialize vp8_mbuverror mmx sse2
vp8_mbuverror_sse2=vp8_mbuverror_xmm

prototype void vp8_subtract_b "struct block *be, struct blockd *bd, int pitch"
specialize vp8_subtract_b mmx sse2 media neon
vp8_subtract_b_media=vp8_subtract_b_armv6

prototype void vp8_subtract_mby "short *diff, unsigned char *src, int src_stride, unsigned char *pred, int pred_stride"
specialize vp8_subtract_mby mmx sse2 media neon
vp8_subtract_mby_media=vp8_subtract_mby_armv6

prototype void vp8_subtract_mbuv "short *diff, unsigned char *usrc, unsigned char *vsrc, int src_stride, unsigned char *upred, unsigned char *vpred, int pred_stride"
specialize vp8_subtract_mbuv mmx sse2 media neon
vp8_subtract_mbuv_media=vp8_subtract_mbuv_armv6

#
# Motion search
#
prototype int vp8_full_search_sad "struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, int sad_per_bit, int distance, struct variance_vtable *fn_ptr, int *mvcost[2], union int_mv *center_mv"
specialize vp8_full_search_sad sse3 sse4_1
vp8_full_search_sad_sse3=vp8_full_search_sadx3
vp8_full_search_sad_sse4_1=vp8_full_search_sadx8

prototype int vp8_refining_search_sad "struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, int sad_per_bit, int distance, struct variance_vtable *fn_ptr, int *mvcost[2], union int_mv *center_mv"
specialize vp8_refining_search_sad sse3
vp8_refining_search_sad_sse3=vp8_refining_search_sadx4

prototype int vp8_diamond_search_sad "struct macroblock *x, struct block *b, struct blockd *d, union int_mv *ref_mv, union int_mv *best_mv, int search_param, int sad_per_bit, int *num00, struct variance_vtable *fn_ptr, int *mvcost[2], union int_mv *center_mv"
vp8_diamond_search_sad_sse3=vp8_diamond_search_sadx4

#
# Alt-ref Noise Reduction (ARNR)
#
if [ "$CONFIG_REALTIME_ONLY" != "yes" ]; then
    prototype void vp8_temporal_filter_apply "unsigned char *frame1, unsigned int stride, unsigned char *frame2, unsigned int block_size, int strength, int filter_weight, unsigned int *accumulator, unsigned short *count"
    specialize vp8_temporal_filter_apply sse2
fi

#
# Pick Loopfilter
#
prototype void vp8_yv12_copy_partial_frame "struct yv12_buffer_config *src_ybc, struct yv12_buffer_config *dst_ybc"
specialize vp8_yv12_copy_partial_frame neon

#
# Denoiser filter
#
if [ "$CONFIG_TEMPORAL_DENOISING" = "yes" ]; then
    prototype int vp8_denoiser_filter "struct yv12_buffer_config* mc_running_avg, struct yv12_buffer_config* running_avg, struct macroblock* signal, unsigned int motion_magnitude2, int y_offset, int uv_offset"
    specialize vp8_denoiser_filter sse2
fi

# End of encoder only functions
fi
