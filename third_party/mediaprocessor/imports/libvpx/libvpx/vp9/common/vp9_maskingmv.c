/*
 ============================================================================
 Name        : vp9_maskingmv.c
 Author      : jimbankoski
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int vp9_sad16x16_sse3(
  unsigned char *src_ptr,
  int  src_stride,
  unsigned char *ref_ptr,
  int  ref_stride,
  int  max_err);

int vp8_growmaskmb_sse3(
  unsigned char *om,
  unsigned char *nm);

void vp8_makemask_sse3(
  unsigned char *y,
  unsigned char *u,
  unsigned char *v,
  unsigned char *ym,
  int yp,
  int uvp,
  int ys,
  int us,
  int vs,
  int yt,
  int ut,
  int vt);

unsigned int vp9_sad16x16_unmasked_wmt(
  unsigned char *src_ptr,
  int  src_stride,
  unsigned char *ref_ptr,
  int  ref_stride,
  unsigned char *mask);

unsigned int vp9_sad16x16_masked_wmt(
  unsigned char *src_ptr,
  int  src_stride,
  unsigned char *ref_ptr,
  int  ref_stride,
  unsigned char *mask);

unsigned int vp8_masked_predictor_wmt(
  unsigned char *masked,
  unsigned char *unmasked,
  int  src_stride,
  unsigned char *dst_ptr,
  int  dst_stride,
  unsigned char *mask);
unsigned int vp8_masked_predictor_uv_wmt(
  unsigned char *masked,
  unsigned char *unmasked,
  int  src_stride,
  unsigned char *dst_ptr,
  int  dst_stride,
  unsigned char *mask);
unsigned int vp8_uv_from_y_mask(
  unsigned char *ymask,
  unsigned char *uvmask);
int yp = 16;
unsigned char sxy[] = {
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 80, 120, 120, 90, 90, 90, 90, 90, 80, 120, 120, 90, 90, 90, 90, 90
};

unsigned char sts[] = {
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};
unsigned char str[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

unsigned char y[] = {
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40,
  40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40,
  60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40,
  60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40,
  60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40,
  40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40,
  40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40, 40,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40,
  40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40, 40,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40,
  40, 40, 40, 60, 60, 60, 60, 40, 40, 40, 40, 60, 60, 60, 60, 40
};
int uvp = 8;
unsigned char u[] = {
  90, 80, 70, 70, 90, 90, 90, 17,
  90, 80, 70, 70, 90, 90, 90, 17,
  84, 70, 70, 90, 90, 90, 17, 17,
  84, 70, 70, 90, 90, 90, 17, 17,
  80, 70, 70, 90, 90, 90, 17, 17,
  90, 80, 70, 70, 90, 90, 90, 17,
  90, 80, 70, 70, 90, 90, 90, 17,
  90, 80, 70, 70, 90, 90, 90, 17
};

unsigned char v[] = {
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80,
  80, 80, 80, 80, 80, 80, 80, 80
};

unsigned char ym[256];
unsigned char uvm[64];
typedef struct {
  unsigned char y;
  unsigned char yt;
  unsigned char u;
  unsigned char ut;
  unsigned char v;
  unsigned char vt;
  unsigned char use;
} COLOR_SEG_ELEMENT;

/*
COLOR_SEG_ELEMENT segmentation[]=
{
    { 60,4,80,17,80,10, 1},
    { 40,4,15,10,80,10, 1},
};
*/

COLOR_SEG_ELEMENT segmentation[] = {
  { 79, 44, 92, 44, 237, 60, 1},
};

unsigned char pixel_mask(unsigned char y, unsigned char u, unsigned char v,
                         COLOR_SEG_ELEMENT sgm[],
                         int c) {
  COLOR_SEG_ELEMENT *s = sgm;
  unsigned char m = 0;
  int i;
  for (i = 0; i < c; i++, s++)
    m |= (abs(y - s->y) < s->yt &&
          abs(u - s->u) < s->ut &&
          abs(v - s->v) < s->vt ? 255 : 0);

  return m;
}
int neighbors[256][8];
int makeneighbors(void) {
  int i, j;
  for (i = 0; i < 256; i++) {
    int r = (i >> 4), c = (i & 15);
    int ni = 0;
    for (j = 0; j < 8; j++)
      neighbors[i][j] = i;
    for (j = 0; j < 256; j++) {
      int nr = (j >> 4), nc = (j & 15);
      if (abs(nr - r) < 2 && abs(nc - c) < 2)
        neighbors[i][ni++] = j;
    }
  }
  return 0;
}
void grow_ymask(unsigned char *ym) {
  unsigned char nym[256];
  int i, j;

  for (i = 0; i < 256; i++) {
    nym[i] = ym[i];
    for (j = 0; j < 8; j++) {
      nym[i] |= ym[neighbors[i][j]];
    }
  }
  for (i = 0; i < 256; i++)
    ym[i] = nym[i];
}

void make_mb_mask(unsigned char *y, unsigned char *u, unsigned char *v,
                  unsigned char *ym, unsigned char *uvm,
                  int yp, int uvp,
                  COLOR_SEG_ELEMENT sgm[],
                  int count) {
  int r, c;
  unsigned char *oym = ym;

  memset(ym, 20, 256);
  for (r = 0; r < 8; r++, uvm += 8, u += uvp, v += uvp, y += (yp + yp), ym += 32)
    for (c = 0; c < 8; c++) {
      int y1 = y[c << 1];
      int u1 = u[c];
      int v1 = v[c];
      int m = pixel_mask(y1, u1, v1, sgm, count);
      uvm[c] = m;
      ym[c << 1] = uvm[c]; // = pixel_mask(y[c<<1],u[c],v[c],sgm,count);
      ym[(c << 1) + 1] = pixel_mask(y[1 + (c << 1)], u[c], v[c], sgm, count);
      ym[(c << 1) + 16] = pixel_mask(y[yp + (c << 1)], u[c], v[c], sgm, count);
      ym[(c << 1) + 17] = pixel_mask(y[1 + yp + (c << 1)], u[c], v[c], sgm, count);
    }
  grow_ymask(oym);
}

int masked_sad(unsigned char *src, int p, unsigned char *dst, int dp,
               unsigned char *ym) {
  int i, j;
  unsigned sad = 0;
  for (i = 0; i < 16; i++, src += p, dst += dp, ym += 16)
    for (j = 0; j < 16; j++)
      if (ym[j])
        sad += abs(src[j] - dst[j]);

  return sad;
}

int compare_masks(unsigned char *sym, unsigned char *ym) {
  int i, j;
  unsigned sad = 0;
  for (i = 0; i < 16; i++, sym += 16, ym += 16)
    for (j = 0; j < 16; j++)
      sad += (sym[j] != ym[j] ? 1 : 0);

  return sad;
}

int unmasked_sad(unsigned char *src, int p, unsigned char *dst, int dp,
                 unsigned char *ym) {
  int i, j;
  unsigned sad = 0;
  for (i = 0; i < 16; i++, src += p, dst += dp, ym += 16)
    for (j = 0; j < 16; j++)
      if (!ym[j])
        sad += abs(src[j] - dst[j]);

  return sad;
}

int masked_motion_search(unsigned char *y, unsigned char *u, unsigned char *v,
                         int yp, int uvp,
                         unsigned char *dy, unsigned char *du, unsigned char *dv,
                         int dyp, int duvp,
                         COLOR_SEG_ELEMENT sgm[],
                         int count,
                         int *mi,
                         int *mj,
                         int *ui,
                         int *uj,
                         int *wm) {
  int i, j;

  unsigned char ym[256];
  unsigned char uvm[64];
  unsigned char dym[256];
  unsigned char duvm[64];
  unsigned int e = 0;
  int beste = 256;
  int bmi = -32, bmj = -32;
  int bui = -32, buj = -32;
  int beste1 = 256;
  int bmi1 = -32, bmj1 = -32;
  int bui1 = -32, buj1 = -32;
  int obeste;

  // first try finding best mask and then unmasked
  beste = 0xffffffff;

  // find best unmasked mv
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    unsigned char *duz = i / 2 * duvp + du;
    unsigned char *dvz = i / 2 * duvp + dv;
    for (j = -32; j < 32; j++) {
      // 0,0  masked destination
      make_mb_mask(dyz + j, duz + j / 2, dvz + j / 2, dym, duvm, dyp, duvp, sgm, count);

      e = unmasked_sad(y, yp, dyz + j, dyp, dym);

      if (e < beste) {
        bui = i;
        buj = j;
        beste = e;
      }
    }
  }
  // bui=0;buj=0;
  // best mv masked destination
  make_mb_mask(dy + bui * dyp + buj, du + bui / 2 * duvp + buj / 2, dv + bui / 2 * duvp + buj / 2,
               dym, duvm, dyp, duvp, sgm, count);

  obeste = beste;
  beste = 0xffffffff;

  // find best masked
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    for (j = -32; j < 32; j++) {
      e = masked_sad(y, yp, dyz + j, dyp, dym);

      if (e < beste) {
        bmi = i;
        bmj = j;
        beste = e;
      }
    }
  }
  beste1 = beste + obeste;
  bmi1 = bmi;
  bmj1 = bmj;
  bui1 = bui;
  buj1 = buj;

  beste = 0xffffffff;
  // source mask
  make_mb_mask(y, u, v, ym, uvm, yp, uvp, sgm, count);

  // find best mask
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    unsigned char *duz = i / 2 * duvp + du;
    unsigned char *dvz = i / 2 * duvp + dv;
    for (j = -32; j < 32; j++) {
      // 0,0  masked destination
      make_mb_mask(dyz + j, duz + j / 2, dvz + j / 2, dym, duvm, dyp, duvp, sgm, count);

      e = compare_masks(ym, dym);

      if (e < beste) {
        bmi = i;
        bmj = j;
        beste = e;
      }
    }
  }


  // best mv masked destination
  make_mb_mask(dy + bmi * dyp + bmj, du + bmi / 2 * duvp + bmj / 2, dv + bmi / 2 * duvp + bmj / 2,
               dym, duvm, dyp, duvp, sgm, count);

  obeste = masked_sad(y, yp, dy + bmi * dyp + bmj, dyp, dym);

  beste = 0xffffffff;

  // find best unmasked mv
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    for (j = -32; j < 32; j++) {
      e = unmasked_sad(y, yp, dyz + j, dyp, dym);

      if (e < beste) {
        bui = i;
        buj = j;
        beste = e;
      }
    }
  }
  beste += obeste;


  if (beste < beste1) {
    *mi = bmi;
    *mj = bmj;
    *ui = bui;
    *uj = buj;
    *wm = 1;
  } else {
    *mi = bmi1;
    *mj = bmj1;
    *ui = bui1;
    *uj = buj1;
    *wm = 0;

  }
  return 0;
}

int predict(unsigned char *src, int p, unsigned char *dst, int dp,
            unsigned char *ym, unsigned char *prd) {
  int i, j;
  for (i = 0; i < 16; i++, src += p, dst += dp, ym += 16, prd += 16)
    for (j = 0; j < 16; j++)
      prd[j] = (ym[j] ? src[j] : dst[j]);
  return 0;
}

int fast_masked_motion_search(unsigned char *y, unsigned char *u, unsigned char *v,
                              int yp, int uvp,
                              unsigned char *dy, unsigned char *du, unsigned char *dv,
                              int dyp, int duvp,
                              COLOR_SEG_ELEMENT sgm[],
                              int count,
                              int *mi,
                              int *mj,
                              int *ui,
                              int *uj,
                              int *wm) {
  int i, j;

  unsigned char ym[256];
  unsigned char ym2[256];
  unsigned char uvm[64];
  unsigned char dym2[256];
  unsigned char dym[256];
  unsigned char duvm[64];
  unsigned int e = 0;
  int beste = 256;
  int bmi = -32, bmj = -32;
  int bui = -32, buj = -32;
  int beste1 = 256;
  int bmi1 = -32, bmj1 = -32;
  int bui1 = -32, buj1 = -32;
  int obeste;

  // first try finding best mask and then unmasked
  beste = 0xffffffff;

#if 0
  for (i = 0; i < 16; i++) {
    unsigned char *dy = i * yp + y;
    for (j = 0; j < 16; j++)
      printf("%2x", dy[j]);
    printf("\n");
  }
  printf("\n");

  for (i = -32; i < 48; i++) {
    unsigned char *dyz = i * dyp + dy;
    for (j = -32; j < 48; j++)
      printf("%2x", dyz[j]);
    printf("\n");
  }
#endif

  // find best unmasked mv
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    unsigned char *duz = i / 2 * duvp + du;
    unsigned char *dvz = i / 2 * duvp + dv;
    for (j = -32; j < 32; j++) {
      // 0,0  masked destination
      vp8_makemask_sse3(dyz + j, duz + j / 2, dvz + j / 2, dym, dyp, duvp,
                        sgm[0].y, sgm[0].u, sgm[0].v,
                        sgm[0].yt, sgm[0].ut, sgm[0].vt);

      vp8_growmaskmb_sse3(dym, dym2);

      e = vp9_sad16x16_unmasked_wmt(y, yp, dyz + j, dyp, dym2);

      if (e < beste) {
        bui = i;
        buj = j;
        beste = e;
      }
    }
  }
  // bui=0;buj=0;
  // best mv masked destination

  vp8_makemask_sse3(dy + bui * dyp + buj, du + bui / 2 * duvp + buj / 2, dv + bui / 2 * duvp + buj / 2,
                    dym, dyp, duvp,
                    sgm[0].y, sgm[0].u, sgm[0].v,
                    sgm[0].yt, sgm[0].ut, sgm[0].vt);

  vp8_growmaskmb_sse3(dym, dym2);

  obeste = beste;
  beste = 0xffffffff;

  // find best masked
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    for (j = -32; j < 32; j++) {
      e = vp9_sad16x16_masked_wmt(y, yp, dyz + j, dyp, dym2);
      if (e < beste) {
        bmi = i;
        bmj = j;
        beste = e;
      }
    }
  }
  beste1 = beste + obeste;
  bmi1 = bmi;
  bmj1 = bmj;
  bui1 = bui;
  buj1 = buj;

  // source mask
  vp8_makemask_sse3(y, u, v,
                    ym, yp, uvp,
                    sgm[0].y, sgm[0].u, sgm[0].v,
                    sgm[0].yt, sgm[0].ut, sgm[0].vt);

  vp8_growmaskmb_sse3(ym, ym2);

  // find best mask
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    unsigned char *duz = i / 2 * duvp + du;
    unsigned char *dvz = i / 2 * duvp + dv;
    for (j = -32; j < 32; j++) {
      // 0,0  masked destination
      vp8_makemask_sse3(dyz + j, duz + j / 2, dvz + j / 2, dym, dyp, duvp,
                        sgm[0].y, sgm[0].u, sgm[0].v,
                        sgm[0].yt, sgm[0].ut, sgm[0].vt);

      vp8_growmaskmb_sse3(dym, dym2);

      e = compare_masks(ym2, dym2);

      if (e < beste) {
        bmi = i;
        bmj = j;
        beste = e;
      }
    }
  }

  vp8_makemask_sse3(dy + bmi * dyp + bmj, du + bmi / 2 * duvp + bmj / 2, dv + bmi / 2 * duvp + bmj / 2,
                    dym, dyp, duvp,
                    sgm[0].y, sgm[0].u, sgm[0].v,
                    sgm[0].yt, sgm[0].ut, sgm[0].vt);

  vp8_growmaskmb_sse3(dym, dym2);

  obeste = vp9_sad16x16_masked_wmt(y, yp, dy + bmi * dyp + bmj, dyp, dym2);

  beste = 0xffffffff;

  // find best unmasked mv
  for (i = -32; i < 32; i++) {
    unsigned char *dyz = i * dyp + dy;
    for (j = -32; j < 32; j++) {
      e = vp9_sad16x16_unmasked_wmt(y, yp, dyz + j, dyp, dym2);

      if (e < beste) {
        bui = i;
        buj = j;
        beste = e;
      }
    }
  }
  beste += obeste;

  if (beste < beste1) {
    *mi = bmi;
    *mj = bmj;
    *ui = bui;
    *uj = buj;
    *wm = 1;
  } else {
    *mi = bmi1;
    *mj = bmj1;
    *ui = bui1;
    *uj = buj1;
    *wm = 0;
    beste = beste1;

  }
  return beste;
}

int predict_all(unsigned char *ym, unsigned char *um, unsigned char *vm,
                int ymp, int uvmp,
                unsigned char *yp, unsigned char *up, unsigned char *vp,
                int ypp, int uvpp,
                COLOR_SEG_ELEMENT sgm[],
                int count,
                int mi,
                int mj,
                int ui,
                int uj,
                int wm) {
  int i, j;
  unsigned char dym[256];
  unsigned char dym2[256];
  unsigned char duvm[64];
  unsigned char *yu = ym, *uu = um, *vu = vm;

  unsigned char *dym3 = dym2;

  ym += mi * ymp + mj;
  um += mi / 2 * uvmp + mj / 2;
  vm += mi / 2 * uvmp + mj / 2;

  yu += ui * ymp + uj;
  uu += ui / 2 * uvmp + uj / 2;
  vu += ui / 2 * uvmp + uj / 2;

  // best mv masked destination
  if (wm)
    vp8_makemask_sse3(ym, um, vm, dym, ymp, uvmp,
                      sgm[0].y, sgm[0].u, sgm[0].v,
                      sgm[0].yt, sgm[0].ut, sgm[0].vt);
  else
    vp8_makemask_sse3(yu, uu, vu, dym, ymp, uvmp,
                      sgm[0].y, sgm[0].u, sgm[0].v,
                      sgm[0].yt, sgm[0].ut, sgm[0].vt);

  vp8_growmaskmb_sse3(dym, dym2);
  vp8_masked_predictor_wmt(ym, yu, ymp, yp, ypp, dym3);
  vp8_uv_from_y_mask(dym3, duvm);
  vp8_masked_predictor_uv_wmt(um, uu, uvmp, up, uvpp, duvm);
  vp8_masked_predictor_uv_wmt(vm, vu, uvmp, vp, uvpp, duvm);

  return 0;
}

unsigned char f0p[1280 * 720 * 3 / 2];
unsigned char f1p[1280 * 720 * 3 / 2];
unsigned char prd[1280 * 720 * 3 / 2];
unsigned char msk[1280 * 720 * 3 / 2];


int mainz(int argc, char *argv[]) {

  FILE *f = fopen(argv[1], "rb");
  FILE *g = fopen(argv[2], "wb");
  int w = atoi(argv[3]), h = atoi(argv[4]);
  int y_stride = w, uv_stride = w / 2;
  int r, c;
  unsigned char *f0 = f0p, *f1 = f1p, *t;
  unsigned char ym[256], uvm[64];
  unsigned char ym2[256], uvm2[64];
  unsigned char ym3[256], uvm3[64];
  int a, b;

  COLOR_SEG_ELEMENT last = { 20, 20, 20, 20, 230, 20, 1}, best;
#if 0
  makeneighbors();
  COLOR_SEG_ELEMENT segmentation[] = {
    { 60, 4, 80, 17, 80, 10, 1},
    { 40, 4, 15, 10, 80, 10, 1},
  };
  make_mb_mask(y, u, v, ym2, uvm2, 16, 8, segmentation, 1);

  vp8_makemask_sse3(y, u, v, ym, (int) 16, (int) 8,
                    (int) segmentation[0].y, (int) segmentation[0].u, (int) segmentation[0].v,
                    segmentation[0].yt, segmentation[0].ut, segmentation[0].vt);

  vp8_growmaskmb_sse3(ym, ym3);

  a = vp9_sad16x16_masked_wmt(str, 16, sts, 16, ym3);
  b = vp9_sad16x16_unmasked_wmt(str, 16, sts, 16, ym3);

  vp8_masked_predictor_wmt(str, sts, 16, ym, 16, ym3);

  vp8_uv_from_y_mask(ym3, uvm3);

  return 4;
#endif
  makeneighbors();


  memset(prd, 128, w * h * 3 / 2);

  fread(f0, w * h * 3 / 2, 1, f);

  while (!feof(f)) {
    unsigned char *ys = f1, *yd = f0, *yp = prd;
    unsigned char *us = f1 + w * h, *ud = f0 + w * h, *up = prd + w * h;
    unsigned char *vs = f1 + w * h * 5 / 4, *vd = f0 + w * h * 5 / 4, *vp = prd + w * h * 5 / 4;
    fread(f1, w * h * 3 / 2, 1, f);

    ys += 32 * y_stride;
    yd += 32 * y_stride;
    yp += 32 * y_stride;
    us += 16 * uv_stride;
    ud += 16 * uv_stride;
    up += 16 * uv_stride;
    vs += 16 * uv_stride;
    vd += 16 * uv_stride;
    vp += 16 * uv_stride;
    for (r = 32; r < h - 32; r += 16,
         ys += 16 * w, yd += 16 * w, yp += 16 * w,
         us += 8 * uv_stride, ud += 8 * uv_stride, up += 8 * uv_stride,
         vs += 8 * uv_stride, vd += 8 * uv_stride, vp += 8 * uv_stride) {
      for (c = 32; c < w - 32; c += 16) {
        int mi, mj, ui, uj, wm;
        int bmi, bmj, bui, buj, bwm;
        unsigned char ym[256];

        if (vp9_sad16x16_sse3(ys + c, y_stride, yd + c, y_stride, 0xffff) == 0)
          bmi = bmj = bui = buj = bwm = 0;
        else {
          COLOR_SEG_ELEMENT cs[5];
          int j;
          unsigned int beste = 0xfffffff;
          unsigned int bestj = 0;

          // try color from last mb segmentation
          cs[0] = last;

          // try color segs from 4 pixels in mb recon as segmentation
          cs[1].y = yd[c + y_stride + 1];
          cs[1].u = ud[c / 2 + uv_stride];
          cs[1].v = vd[c / 2 + uv_stride];
          cs[1].yt = cs[1].ut = cs[1].vt = 20;
          cs[2].y = yd[c + w + 14];
          cs[2].u = ud[c / 2 + uv_stride + 7];
          cs[2].v = vd[c / 2 + uv_stride + 7];
          cs[2].yt = cs[2].ut = cs[2].vt = 20;
          cs[3].y = yd[c + w * 14 + 1];
          cs[3].u = ud[c / 2 + uv_stride * 7];
          cs[3].v = vd[c / 2 + uv_stride * 7];
          cs[3].yt = cs[3].ut = cs[3].vt = 20;
          cs[4].y = yd[c + w * 14 + 14];
          cs[4].u = ud[c / 2 + uv_stride * 7 + 7];
          cs[4].v = vd[c / 2 + uv_stride * 7 + 7];
          cs[4].yt = cs[4].ut = cs[4].vt = 20;

          for (j = 0; j < 5; j++) {
            int e;

            e = fast_masked_motion_search(
                  ys + c, us + c / 2, vs + c / 2, y_stride, uv_stride,
                  yd + c, ud + c / 2, vd + c / 2, y_stride, uv_stride,
                  &cs[j], 1, &mi, &mj, &ui, &uj, &wm);

            if (e < beste) {
              bmi = mi;
              bmj = mj;
              bui = ui;
              buj = uj, bwm = wm;
              bestj = j;
              beste = e;
            }
          }
          best = cs[bestj];
          // best = segmentation[0];
          last = best;
        }
        predict_all(yd + c, ud + c / 2, vd + c / 2, w, uv_stride,
                    yp + c, up + c / 2, vp + c / 2, w, uv_stride,
                    &best, 1, bmi, bmj, bui, buj, bwm);

      }
    }
    fwrite(prd, w * h * 3 / 2, 1, g);
    t = f0;
    f0 = f1;
    f1 = t;

  }
  fclose(f);
  fclose(g);
  return 0;
}
