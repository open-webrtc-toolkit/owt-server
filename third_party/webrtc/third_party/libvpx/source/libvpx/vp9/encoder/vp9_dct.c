/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <assert.h>
#include <math.h>
#include "./vpx_config.h"
#include "vp9/common/vp9_systemdependent.h"

#include "vp9/common/vp9_blockd.h"

// TODO: these transforms can be converted into integer forms to reduce
//       the complexity
static const float dct_4[16] = {
  0.500000000000000,  0.500000000000000,  0.500000000000000,  0.500000000000000,
  0.653281482438188,  0.270598050073099, -0.270598050073099, -0.653281482438188,
  0.500000000000000, -0.500000000000000, -0.500000000000000,  0.500000000000000,
  0.270598050073099, -0.653281482438188,  0.653281482438188, -0.270598050073099
};

static const float adst_4[16] = {
  0.228013428883779,  0.428525073124360,  0.577350269189626,  0.656538502008139,
  0.577350269189626,  0.577350269189626,  0.000000000000000, -0.577350269189626,
  0.656538502008139, -0.228013428883779, -0.577350269189626,  0.428525073124359,
  0.428525073124360, -0.656538502008139,  0.577350269189626, -0.228013428883779
};

static const float dct_8[64] = {
  0.353553390593274,   0.353553390593274,   0.353553390593274,   0.353553390593274,
  0.353553390593274,   0.353553390593274,   0.353553390593274,   0.353553390593274,
  0.490392640201615,   0.415734806151273,   0.277785116509801,   0.097545161008064,
 -0.097545161008064,  -0.277785116509801,  -0.415734806151273,  -0.490392640201615,
  0.461939766255643,   0.191341716182545,  -0.191341716182545,  -0.461939766255643,
 -0.461939766255643,  -0.191341716182545,   0.191341716182545,   0.461939766255643,
  0.415734806151273,  -0.097545161008064,  -0.490392640201615,  -0.277785116509801,
  0.277785116509801,   0.490392640201615,   0.097545161008064,  -0.415734806151273,
  0.353553390593274,  -0.353553390593274,  -0.353553390593274,   0.353553390593274,
  0.353553390593274,  -0.353553390593274,  -0.353553390593274,   0.353553390593274,
  0.277785116509801,  -0.490392640201615,   0.097545161008064,   0.415734806151273,
 -0.415734806151273,  -0.097545161008064,   0.490392640201615,  -0.277785116509801,
  0.191341716182545,  -0.461939766255643,   0.461939766255643,  -0.191341716182545,
 -0.191341716182545,   0.461939766255643,  -0.461939766255643,   0.191341716182545,
  0.097545161008064,  -0.277785116509801,   0.415734806151273,  -0.490392640201615,
  0.490392640201615,  -0.415734806151273,   0.277785116509801,  -0.097545161008064
};

static const float adst_8[64] = {
  0.089131608307533,   0.175227946595735,   0.255357107325376,   0.326790388032145,
  0.387095214016349,   0.434217976756762,   0.466553967085785,   0.483002021635509,
  0.255357107325376,   0.434217976756762,   0.483002021635509,   0.387095214016349,
  0.175227946595735,  -0.089131608307533,  -0.326790388032145,  -0.466553967085785,
  0.387095214016349,   0.466553967085785,   0.175227946595735,  -0.255357107325376,
 -0.483002021635509,  -0.326790388032145,   0.089131608307533,   0.434217976756762,
  0.466553967085785,   0.255357107325376,  -0.326790388032145,  -0.434217976756762,
  0.089131608307533,   0.483002021635509,   0.175227946595735,  -0.387095214016348,
  0.483002021635509,  -0.089131608307533,  -0.466553967085785,   0.175227946595735,
  0.434217976756762,  -0.255357107325376,  -0.387095214016348,   0.326790388032145,
  0.434217976756762,  -0.387095214016348,  -0.089131608307533,   0.466553967085786,
 -0.326790388032145,  -0.175227946595735,   0.483002021635509,  -0.255357107325375,
  0.326790388032145,  -0.483002021635509,   0.387095214016349,  -0.089131608307534,
 -0.255357107325377,   0.466553967085785,  -0.434217976756762,   0.175227946595736,
  0.175227946595735,  -0.326790388032145,   0.434217976756762,  -0.483002021635509,
  0.466553967085785,  -0.387095214016348,   0.255357107325376,  -0.089131608307532
};

/* Converted the transforms to integers. */
static const int16_t dct_i4[16] = {
  16384,  16384,  16384,  16384,
  21407,   8867,  -8867, -21407,
  16384, -16384, -16384,  16384,
   8867, -21407,  21407,  -8867
};

static const int16_t adst_i4[16] = {
   7472,  14042,  18919,  21513,
  18919,  18919,      0, -18919,
  21513,  -7472, -18919,  14042,
  14042, -21513,  18919,  -7472
};

static const int16_t dct_i8[64] = {
   11585,  11585,  11585,  11585,
   11585,  11585,  11585,  11585,
   16069,  13623,   9102,   3196,
   -3196,  -9102, -13623, -16069,
   15137,   6270,  -6270, -15137,
  -15137,  -6270,   6270,  15137,
   13623,  -3196, -16069,  -9102,
    9102,  16069,   3196, -13623,
   11585, -11585, -11585,  11585,
   11585, -11585, -11585,  11585,
    9102, -16069,   3196,  13623,
  -13623,  -3196,  16069,  -9102,
    6270, -15137,  15137,  -6270,
   -6270,  15137, -15137,   6270,
    3196,  -9102,  13623, -16069,
   16069, -13623,   9102,  -3196
};

static const int16_t adst_i8[64] = {
    2921,   5742,   8368,  10708,
   12684,  14228,  15288,  15827,
    8368,  14228,  15827,  12684,
    5742,  -2921, -10708, -15288,
   12684,  15288,   5742,  -8368,
  -15827, -10708,   2921,  14228,
   15288,   8368, -10708, -14228,
    2921,  15827,   5742, -12684,
   15827,  -2921, -15288,   5742,
   14228,  -8368, -12684,  10708,
   14228, -12684,  -2921,  15288,
  -10708,  -5742,  15827,  -8368,
   10708, -15827,  12684,  -2921,
   -8368,  15288, -14228,   5742,
    5742, -10708,  14228, -15827,
   15288, -12684,   8368,  -2921
};

static const float dct_16[256] = {
  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,
  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,
  0.351851,  0.338330,  0.311806,  0.273300,  0.224292,  0.166664,  0.102631,  0.034654,
 -0.034654, -0.102631, -0.166664, -0.224292, -0.273300, -0.311806, -0.338330, -0.351851,
  0.346760,  0.293969,  0.196424,  0.068975, -0.068975, -0.196424, -0.293969, -0.346760,
 -0.346760, -0.293969, -0.196424, -0.068975,  0.068975,  0.196424,  0.293969,  0.346760,
  0.338330,  0.224292,  0.034654, -0.166664, -0.311806, -0.351851, -0.273300, -0.102631,
  0.102631,  0.273300,  0.351851,  0.311806,  0.166664, -0.034654, -0.224292, -0.338330,
  0.326641,  0.135299, -0.135299, -0.326641, -0.326641, -0.135299,  0.135299,  0.326641,
  0.326641,  0.135299, -0.135299, -0.326641, -0.326641, -0.135299,  0.135299,  0.326641,
  0.311806,  0.034654, -0.273300, -0.338330, -0.102631,  0.224292,  0.351851,  0.166664,
 -0.166664, -0.351851, -0.224292,  0.102631,  0.338330,  0.273300, -0.034654, -0.311806,
  0.293969, -0.068975, -0.346760, -0.196424,  0.196424,  0.346760,  0.068975, -0.293969,
 -0.293969,  0.068975,  0.346760,  0.196424, -0.196424, -0.346760, -0.068975,  0.293969,
  0.273300, -0.166664, -0.338330,  0.034654,  0.351851,  0.102631, -0.311806, -0.224292,
  0.224292,  0.311806, -0.102631, -0.351851, -0.034654,  0.338330,  0.166664, -0.273300,
  0.250000, -0.250000, -0.250000,  0.250000,  0.250000, -0.250000, -0.250000,  0.250000,
  0.250000, -0.250000, -0.250000,  0.250000,  0.250000, -0.250000, -0.250000,  0.250000,
  0.224292, -0.311806, -0.102631,  0.351851, -0.034654, -0.338330,  0.166664,  0.273300,
 -0.273300, -0.166664,  0.338330,  0.034654, -0.351851,  0.102631,  0.311806, -0.224292,
  0.196424, -0.346760,  0.068975,  0.293969, -0.293969, -0.068975,  0.346760, -0.196424,
 -0.196424,  0.346760, -0.068975, -0.293969,  0.293969,  0.068975, -0.346760,  0.196424,
  0.166664, -0.351851,  0.224292,  0.102631, -0.338330,  0.273300,  0.034654, -0.311806,
  0.311806, -0.034654, -0.273300,  0.338330, -0.102631, -0.224292,  0.351851, -0.166664,
  0.135299, -0.326641,  0.326641, -0.135299, -0.135299,  0.326641, -0.326641,  0.135299,
  0.135299, -0.326641,  0.326641, -0.135299, -0.135299,  0.326641, -0.326641,  0.135299,
  0.102631, -0.273300,  0.351851, -0.311806,  0.166664,  0.034654, -0.224292,  0.338330,
 -0.338330,  0.224292, -0.034654, -0.166664,  0.311806, -0.351851,  0.273300, -0.102631,
  0.068975, -0.196424,  0.293969, -0.346760,  0.346760, -0.293969,  0.196424, -0.068975,
 -0.068975,  0.196424, -0.293969,  0.346760, -0.346760,  0.293969, -0.196424,  0.068975,
  0.034654, -0.102631,  0.166664, -0.224292,  0.273300, -0.311806,  0.338330, -0.351851,
  0.351851, -0.338330,  0.311806, -0.273300,  0.224292, -0.166664,  0.102631, -0.034654
};

static const float adst_16[256] = {
  0.033094,  0.065889,  0.098087,  0.129396,  0.159534,  0.188227,  0.215215,  0.240255,
  0.263118,  0.283599,  0.301511,  0.316693,  0.329007,  0.338341,  0.344612,  0.347761,
  0.098087,  0.188227,  0.263118,  0.316693,  0.344612,  0.344612,  0.316693,  0.263118,
  0.188227,  0.098087,  0.000000, -0.098087, -0.188227, -0.263118, -0.316693, -0.344612,
  0.159534,  0.283599,  0.344612,  0.329007,  0.240255,  0.098087, -0.065889, -0.215215,
 -0.316693, -0.347761, -0.301511, -0.188227, -0.033094,  0.129396,  0.263118,  0.338341,
  0.215215,  0.338341,  0.316693,  0.159534, -0.065889, -0.263118, -0.347761, -0.283599,
 -0.098087,  0.129396,  0.301511,  0.344612,  0.240255,  0.033094, -0.188227, -0.329007,
  0.263118,  0.344612,  0.188227, -0.098087, -0.316693, -0.316693, -0.098087,  0.188227,
  0.344612,  0.263118,  0.000000, -0.263118, -0.344612, -0.188227,  0.098087,  0.316693,
  0.301511,  0.301511,  0.000000, -0.301511, -0.301511, -0.000000,  0.301511,  0.301511,
  0.000000, -0.301511, -0.301511, -0.000000,  0.301511,  0.301511,  0.000000, -0.301511,
  0.329007,  0.215215, -0.188227, -0.338341, -0.033094,  0.316693,  0.240255, -0.159534,
 -0.344612, -0.065889,  0.301511,  0.263118, -0.129396, -0.347761, -0.098087,  0.283599,
  0.344612,  0.098087, -0.316693, -0.188227,  0.263118,  0.263118, -0.188227, -0.316693,
  0.098087,  0.344612,  0.000000, -0.344612, -0.098087,  0.316693,  0.188227, -0.263118,
  0.347761, -0.033094, -0.344612,  0.065889,  0.338341, -0.098087, -0.329007,  0.129396,
  0.316693, -0.159534, -0.301511,  0.188227,  0.283599, -0.215215, -0.263118,  0.240255,
  0.338341, -0.159534, -0.263118,  0.283599,  0.129396, -0.344612,  0.033094,  0.329007,
 -0.188227, -0.240255,  0.301511,  0.098087, -0.347761,  0.065889,  0.316693, -0.215215,
  0.316693, -0.263118, -0.098087,  0.344612, -0.188227, -0.188227,  0.344612, -0.098087,
 -0.263118,  0.316693,  0.000000, -0.316693,  0.263118,  0.098087, -0.344612,  0.188227,
  0.283599, -0.329007,  0.098087,  0.215215, -0.347761,  0.188227,  0.129396, -0.338341,
  0.263118,  0.033094, -0.301511,  0.316693, -0.065889, -0.240255,  0.344612, -0.159534,
  0.240255, -0.347761,  0.263118, -0.033094, -0.215215,  0.344612, -0.283599,  0.065889,
  0.188227, -0.338341,  0.301511, -0.098087, -0.159534,  0.329007, -0.316693,  0.129396,
  0.188227, -0.316693,  0.344612, -0.263118,  0.098087,  0.098087, -0.263118,  0.344612,
 -0.316693,  0.188227,  0.000000, -0.188227,  0.316693, -0.344612,  0.263118, -0.098087,
  0.129396, -0.240255,  0.316693, -0.347761,  0.329007, -0.263118,  0.159534, -0.033094,
 -0.098087,  0.215215, -0.301511,  0.344612, -0.338341,  0.283599, -0.188227,  0.065889,
  0.065889, -0.129396,  0.188227, -0.240255,  0.283599, -0.316693,  0.338341, -0.347761,
  0.344612, -0.329007,  0.301511, -0.263118,  0.215215, -0.159534,  0.098087, -0.033094
};

/* Converted the transforms to integers. */
static const int16_t dct_i16[256] = {
    8192,   8192,   8192,   8192,   8192,   8192,   8192,   8192,
    8192,   8192,   8192,   8192,   8192,   8192,   8192,   8192,
   11529,  11086,  10217,   8955,   7350,   5461,   3363,   1136,
   -1136,  -3363,  -5461,  -7350,  -8955, -10217, -11086, -11529,
   11363,   9633,   6436,   2260,  -2260,  -6436,  -9633, -11363,
  -11363,  -9633,  -6436,  -2260,   2260,   6436,   9633,  11363,
   11086,   7350,   1136,  -5461, -10217, -11529,  -8955,  -3363,
    3363,   8955,  11529,  10217,   5461,  -1136,  -7350, -11086,
   10703,   4433,  -4433, -10703, -10703,  -4433,   4433,  10703,
   10703,   4433,  -4433, -10703, -10703,  -4433,   4433,  10703,
   10217,   1136,  -8955, -11086,  -3363,   7350,  11529,   5461,
   -5461, -11529,  -7350,   3363,  11086,   8955,  -1136, -10217,
    9633,  -2260, -11363,  -6436,   6436,  11363,   2260,  -9633,
   -9633,   2260,  11363,   6436,  -6436, -11363,  -2260,   9633,
    8955,  -5461, -11086,   1136,  11529,   3363, -10217,  -7350,
    7350,  10217,  -3363, -11529,  -1136,  11086,   5461,  -8955,
    8192,  -8192,  -8192,   8192,   8192,  -8192,  -8192,   8192,
    8192,  -8192,  -8192,   8192,   8192,  -8192,  -8192,   8192,
    7350, -10217,  -3363,  11529,  -1136, -11086,   5461,   8955,
   -8955,  -5461,  11086,   1136, -11529,   3363,  10217,  -7350,
    6436, -11363,   2260,   9633,  -9633,  -2260,  11363,  -6436,
   -6436,  11363,  -2260,  -9633,   9633,   2260, -11363,   6436,
    5461, -11529,   7350,   3363, -11086,   8955,   1136, -10217,
   10217,  -1136,  -8955,  11086,  -3363,  -7350,  11529,  -5461,
    4433, -10703,  10703,  -4433,  -4433,  10703, -10703,   4433,
    4433, -10703,  10703,  -4433,  -4433,  10703, -10703,   4433,
    3363,  -8955,  11529, -10217,   5461,   1136,  -7350,  11086,
  -11086,   7350,  -1136,  -5461,  10217, -11529,   8955,  -3363,
    2260,  -6436,   9633, -11363,  11363,  -9633,   6436,  -2260,
   -2260,   6436,  -9633,  11363, -11363,   9633,  -6436,   2260,
    1136,  -3363,   5461,  -7350,   8955, -10217,  11086, -11529,
   11529, -11086,  10217,  -8955,   7350,  -5461,   3363,  -1136
};

static const int16_t adst_i16[256] = {
    1084,   2159,   3214,   4240,   5228,   6168,   7052,   7873,
    8622,   9293,   9880,  10377,  10781,  11087,  11292,  11395,
    3214,   6168,   8622,  10377,  11292,  11292,  10377,   8622,
    6168,   3214,      0,  -3214,  -6168,  -8622, -10377, -11292,
    5228,   9293,  11292,  10781,   7873,   3214,  -2159,  -7052,
  -10377, -11395,  -9880,  -6168,  -1084,   4240,   8622,  11087,
    7052,  11087,  10377,   5228,  -2159,  -8622, -11395,  -9293,
   -3214,   4240,   9880,  11292,   7873,   1084,  -6168, -10781,
    8622,  11292,   6168,  -3214, -10377, -10377,  -3214,   6168,
   11292,   8622,      0,  -8622, -11292,  -6168,   3214,  10377,
    9880,   9880,      0,  -9880,  -9880,      0,   9880,   9880,
       0,  -9880,  -9880,      0,   9880,   9880,      0,  -9880,
   10781,   7052,  -6168, -11087,  -1084,  10377,   7873,  -5228,
  -11292,  -2159,   9880,   8622,  -4240, -11395,  -3214,   9293,
   11292,   3214, -10377,  -6168,   8622,   8622,  -6168, -10377,
    3214,  11292,      0, -11292,  -3214,  10377,   6168,  -8622,
   11395,  -1084, -11292,   2159,  11087,  -3214, -10781,   4240,
   10377,  -5228,  -9880,   6168,   9293,  -7052,  -8622,   7873,
   11087,  -5228,  -8622,   9293,   4240, -11292,   1084,  10781,
   -6168,  -7873,   9880,   3214, -11395,   2159,  10377,  -7052,
   10377,  -8622,  -3214,  11292,  -6168,  -6168,  11292,  -3214,
   -8622,  10377,      0, -10377,   8622,   3214, -11292,   6168,
    9293, -10781,   3214,   7052, -11395,   6168,   4240, -11087,
    8622,   1084,  -9880,  10377,  -2159,  -7873,  11292,  -5228,
    7873, -11395,   8622,  -1084,  -7052,  11292,  -9293,   2159,
    6168, -11087,   9880,  -3214,  -5228,  10781, -10377,   4240,
    6168, -10377,  11292,  -8622,   3214,   3214,  -8622,  11292,
  -10377,   6168,      0,  -6168,  10377, -11292,   8622,  -3214,
    4240,  -7873,  10377, -11395,  10781,  -8622,   5228,  -1084,
   -3214,   7052,  -9880,  11292, -11087,   9293,  -6168,   2159,
    2159,  -4240,   6168,  -7873,   9293, -10377,  11087, -11395,
   11292, -10781,   9880,  -8622,   7052,  -5228,   3214,  -1084
};

static const int xC1S7 = 16069;
static const int xC2S6 = 15137;
static const int xC3S5 = 13623;
static const int xC4S4 = 11585;
static const int xC5S3 =  9102;
static const int xC6S2 =  6270;
static const int xC7S1 =  3196;

#define SHIFT_BITS 14
#define DOROUND(X) X += (1<<(SHIFT_BITS-1));

#define FINAL_SHIFT 3
#define FINAL_ROUNDING (1<<(FINAL_SHIFT -1))
#define IN_SHIFT (FINAL_SHIFT+1)


void vp9_short_fdct8x8_c(short *InputData, short *OutputData, int pitch) {
  int loop;
  int short_pitch = pitch >> 1;
  int is07, is12, is34, is56;
  int is0734, is1256;
  int id07, id12, id34, id56;
  int irot_input_x, irot_input_y;
  int icommon_product1;      // Re-used product  (c4s4 * (s12 - s56))
  int icommon_product2;      // Re-used product  (c4s4 * (d12 + d56))
  int temp1, temp2;          // intermediate variable for computation

  int  InterData[64];
  int  *ip = InterData;
  short *op = OutputData;

  for (loop = 0; loop < 8; loop++) {
    // Pre calculate some common sums and differences.
    is07 = (InputData[0] + InputData[7]) << IN_SHIFT;
    is12 = (InputData[1] + InputData[2]) << IN_SHIFT;
    is34 = (InputData[3] + InputData[4]) << IN_SHIFT;
    is56 = (InputData[5] + InputData[6]) << IN_SHIFT;
    id07 = (InputData[0] - InputData[7]) << IN_SHIFT;
    id12 = (InputData[1] - InputData[2]) << IN_SHIFT;
    id34 = (InputData[3] - InputData[4]) << IN_SHIFT;
    id56 = (InputData[5] - InputData[6]) << IN_SHIFT;

    is0734 = is07 + is34;
    is1256 = is12 + is56;

    // Pre-Calculate some common product terms.
    icommon_product1 = xC4S4 * (is12 - is56);
    DOROUND(icommon_product1)
    icommon_product1 >>= SHIFT_BITS;

    icommon_product2 = xC4S4 * (id12 + id56);
    DOROUND(icommon_product2)
    icommon_product2 >>= SHIFT_BITS;


    ip[0] = (xC4S4 * (is0734 + is1256));
    DOROUND(ip[0]);
    ip[0] >>= SHIFT_BITS;

    ip[4] = (xC4S4 * (is0734 - is1256));
    DOROUND(ip[4]);
    ip[4] >>= SHIFT_BITS;

    // Define inputs to rotation for outputs 2 and 6
    irot_input_x = id12 - id56;
    irot_input_y = is07 - is34;

    // Apply rotation for outputs 2 and 6.
    temp1 = xC6S2 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC2S6 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    ip[2] = temp1 + temp2;

    temp1 = xC6S2 * irot_input_y;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC2S6 * irot_input_x;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    ip[6] = temp1 - temp2;

    // Define inputs to rotation for outputs 1 and 7
    irot_input_x = icommon_product1 + id07;
    irot_input_y = -(id34 + icommon_product2);

    // Apply rotation for outputs 1 and 7.
    temp1 = xC1S7 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC7S1 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    ip[1] = temp1 - temp2;

    temp1 = xC7S1 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC1S7 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    ip[7] = temp1 + temp2;

    // Define inputs to rotation for outputs 3 and 5
    irot_input_x = id07 - icommon_product1;
    irot_input_y = id34 - icommon_product2;

    // Apply rotation for outputs 3 and 5.
    temp1 = xC3S5 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC5S3 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    ip[3] = temp1 - temp2;


    temp1 = xC5S3 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC3S5 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    ip[5] = temp1 + temp2;

    // Increment data pointer for next row
    InputData += short_pitch;
    ip += 8;
  }

  // Performed DCT on rows, now transform the columns
  ip = InterData;
  for (loop = 0; loop < 8; loop++) {
    // Pre calculate some common sums and differences.
    is07 = ip[0 * 8] + ip[7 * 8];
    is12 = ip[1 * 8] + ip[2 * 8];
    is34 = ip[3 * 8] + ip[4 * 8];
    is56 = ip[5 * 8] + ip[6 * 8];

    id07 = ip[0 * 8] - ip[7 * 8];
    id12 = ip[1 * 8] - ip[2 * 8];
    id34 = ip[3 * 8] - ip[4 * 8];
    id56 = ip[5 * 8] - ip[6 * 8];

    is0734 = is07 + is34;
    is1256 = is12 + is56;

    // Pre-Calculate some common product terms
    icommon_product1 = xC4S4 * (is12 - is56);
    icommon_product2 = xC4S4 * (id12 + id56);
    DOROUND(icommon_product1)
    DOROUND(icommon_product2)
    icommon_product1 >>= SHIFT_BITS;
    icommon_product2 >>= SHIFT_BITS;


    temp1 = xC4S4 * (is0734 + is1256);
    temp2 = xC4S4 * (is0734 - is1256);
    DOROUND(temp1);
    DOROUND(temp2);
    temp1 >>= SHIFT_BITS;

    temp2 >>= SHIFT_BITS;
    op[0 * 8] = (temp1 + FINAL_ROUNDING) >> FINAL_SHIFT;
    op[4 * 8] = (temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;

    // Define inputs to rotation for outputs 2 and 6
    irot_input_x = id12 - id56;
    irot_input_y = is07 - is34;

    // Apply rotation for outputs 2 and 6.
    temp1 = xC6S2 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC2S6 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    op[2 * 8] = (temp1 + temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;

    temp1 = xC6S2 * irot_input_y;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC2S6 * irot_input_x;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    op[6 * 8] = (temp1 - temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;

    // Define inputs to rotation for outputs 1 and 7
    irot_input_x = icommon_product1 + id07;
    irot_input_y = -(id34 + icommon_product2);

    // Apply rotation for outputs 1 and 7.
    temp1 = xC1S7 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC7S1 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    op[1 * 8] = (temp1 - temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;

    temp1 = xC7S1 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC1S7 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    op[7 * 8] = (temp1 + temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;

    // Define inputs to rotation for outputs 3 and 5
    irot_input_x = id07 - icommon_product1;
    irot_input_y = id34 - icommon_product2;

    // Apply rotation for outputs 3 and 5.
    temp1 = xC3S5 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC5S3 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    op[3 * 8] = (temp1 - temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;


    temp1 = xC5S3 * irot_input_x;
    DOROUND(temp1);
    temp1 >>= SHIFT_BITS;
    temp2 = xC3S5 * irot_input_y;
    DOROUND(temp2);
    temp2 >>= SHIFT_BITS;
    op[5 * 8] = (temp1 + temp2 + FINAL_ROUNDING) >> FINAL_SHIFT;

    // Increment data pointer for next column.
    ip++;
    op++;
  }
}

void vp9_short_fhaar2x2_c(short *input, short *output, int pitch) {
  /* [1 1; 1 -1] orthogonal transform */
  /* use position: 0,1, 4, 8 */
  int i;
  short *ip1 = input;
  short *op1 = output;
  for (i = 0; i < 16; i++) {
    op1[i] = 0;
  }

  op1[0] = (ip1[0] + ip1[1] + ip1[4] + ip1[8] + 1) >> 1;
  op1[1] = (ip1[0] - ip1[1] + ip1[4] - ip1[8]) >> 1;
  op1[4] = (ip1[0] + ip1[1] - ip1[4] - ip1[8]) >> 1;
  op1[8] = (ip1[0] - ip1[1] - ip1[4] + ip1[8]) >> 1;
}

/* For test */
#define TEST_INT 1
#if TEST_INT
#define vp9_fht_int_c vp9_fht_c
#else
#define vp9_fht_float_c vp9_fht_c
#endif

void vp9_fht_float_c(const int16_t *input, int pitch, int16_t *output,
               TX_TYPE tx_type, int tx_dim) {
  vp9_clear_system_state();  // Make it simd safe : __asm emms;
  {
    int i, j, k;
    float bufa[256], bufb[256];  // buffers are for floating-point test purpose
                                 // the implementation could be simplified in
                                 // conjunction with integer transform
    const int16_t *ip = input;
    int16_t *op = output;

    float *pfa = &bufa[0];
    float *pfb = &bufb[0];

    // pointers to vertical and horizontal transforms
    const float *ptv, *pth;

    assert(tx_type != DCT_DCT);
    // load and convert residual array into floating-point
    for (j = 0; j < tx_dim; j++) {
      for (i = 0; i < tx_dim; i++) {
        pfa[i] = (float)ip[i];
      }
      pfa += tx_dim;
      ip  += pitch / 2;
    }

    // vertical transformation
    pfa = &bufa[0];
    pfb = &bufb[0];

    switch (tx_type) {
      case ADST_ADST :
      case ADST_DCT  :
        ptv = (tx_dim == 4) ? &adst_4[0] :
                              ((tx_dim == 8) ? &adst_8[0] : &adst_16[0]);
        break;

      default :
        ptv = (tx_dim == 4) ? &dct_4[0] :
                              ((tx_dim == 8) ? &dct_8[0] : &dct_16[0]);
        break;
    }

    for (j = 0; j < tx_dim; j++) {
      for (i = 0; i < tx_dim; i++) {
        pfb[i] = 0;
        for (k = 0; k < tx_dim; k++) {
          pfb[i] += ptv[k] * pfa[(k * tx_dim)];
        }
        pfa += 1;
      }
      pfb += tx_dim;
      ptv += tx_dim;
      pfa = &bufa[0];
    }

    // horizontal transformation
    pfa = &bufa[0];
    pfb = &bufb[0];

    switch (tx_type) {
      case ADST_ADST :
      case  DCT_ADST :
        pth = (tx_dim == 4) ? &adst_4[0] :
                              ((tx_dim == 8) ? &adst_8[0] : &adst_16[0]);
        break;

      default :
        pth = (tx_dim == 4) ? &dct_4[0] :
                              ((tx_dim == 8) ? &dct_8[0] : &dct_16[0]);
        break;
    }

    for (j = 0; j < tx_dim; j++) {
      for (i = 0; i < tx_dim; i++) {
        pfa[i] = 0;
        for (k = 0; k < tx_dim; k++) {
          pfa[i] += pfb[k] * pth[k];
        }
        pth += tx_dim;
      }

      pfa += tx_dim;
      pfb += tx_dim;
      // pth -= tx_dim * tx_dim;

      switch (tx_type) {
        case ADST_ADST :
        case  DCT_ADST :
          pth = (tx_dim == 4) ? &adst_4[0] :
                                ((tx_dim == 8) ? &adst_8[0] : &adst_16[0]);
          break;

        default :
          pth = (tx_dim == 4) ? &dct_4[0] :
                                ((tx_dim == 8) ? &dct_8[0] : &dct_16[0]);
          break;
      }
    }

    // convert to short integer format and load BLOCKD buffer
    op = output;
    pfa = &bufa[0];

    for (j = 0; j < tx_dim; j++) {
      for (i = 0; i < tx_dim; i++) {
        op[i] = (pfa[i] > 0 ) ? (int16_t)( 8 * pfa[i] + 0.49) :
                                     -(int16_t)(- 8 * pfa[i] + 0.49);
      }
      op  += tx_dim;
      pfa += tx_dim;
    }
  }
  vp9_clear_system_state();  // Make it simd safe : __asm emms;
}

/* Converted the transforms to integer form. */
#define VERTICAL_SHIFT 11
#define VERTICAL_ROUNDING ((1 << (VERTICAL_SHIFT - 1)) - 1)
#define HORIZONTAL_SHIFT 16
#define HORIZONTAL_ROUNDING ((1 << (HORIZONTAL_SHIFT - 1)) - 1)
void vp9_fht_int_c(const int16_t *input, int pitch, int16_t *output,
                   TX_TYPE tx_type, int tx_dim) {
  int i, j, k;
  int16_t imbuf[256];

  const int16_t *ip = input;
  int16_t *op = output;
  int16_t *im = &imbuf[0];

  /* pointers to vertical and horizontal transforms. */
  const int16_t *ptv = NULL, *pth = NULL;

  switch (tx_type) {
    case ADST_ADST :
      ptv = pth = (tx_dim == 4) ? &adst_i4[0]
                                  : ((tx_dim == 8) ? &adst_i8[0]
                                                     : &adst_i16[0]);
      break;
    case ADST_DCT  :
      ptv = (tx_dim == 4) ? &adst_i4[0]
                            : ((tx_dim == 8) ? &adst_i8[0] : &adst_i16[0]);
      pth = (tx_dim == 4) ? &dct_i4[0]
                            : ((tx_dim == 8) ? &dct_i8[0] : &dct_i16[0]);
      break;
    case  DCT_ADST :
      ptv = (tx_dim == 4) ? &dct_i4[0]
                            : ((tx_dim == 8) ? &dct_i8[0] : &dct_i16[0]);
      pth = (tx_dim == 4) ? &adst_i4[0]
                            : ((tx_dim == 8) ? &adst_i8[0] : &adst_i16[0]);
      break;
    case  DCT_DCT :
      ptv = pth = (tx_dim == 4) ? &dct_i4[0]
                                  : ((tx_dim == 8) ? &dct_i8[0] : &dct_i16[0]);
      break;
    default:
      assert(0);
      break;
  }

  /* vertical transformation */
  for (j = 0; j < tx_dim; j++) {
    for (i = 0; i < tx_dim; i++) {
      int temp = 0;

      for (k = 0; k < tx_dim; k++) {
        temp += ptv[k] * ip[(k * (pitch >> 1))];
      }

      im[i] = (int16_t)((temp + VERTICAL_ROUNDING) >> VERTICAL_SHIFT);
      ip++;
    }
    im += tx_dim;  // 16
    ptv += tx_dim;
    ip = input;
  }

  /* horizontal transformation */
  im = &imbuf[0];

  for (j = 0; j < tx_dim; j++) {
    const int16_t *pthc = pth;

    for (i = 0; i < tx_dim; i++) {
      int temp = 0;

      for (k = 0; k < tx_dim; k++) {
        temp += im[k] * pthc[k];
      }

      op[i] = (int16_t)((temp + HORIZONTAL_ROUNDING) >> HORIZONTAL_SHIFT);
      pthc += tx_dim;
    }

    im += tx_dim;  // 16
    op += tx_dim;
  }
}

void vp9_short_fdct4x4_c(short *input, short *output, int pitch) {
  int i;
  int a1, b1, c1, d1;
  short *ip = input;
  short *op = output;

  for (i = 0; i < 4; i++) {
    a1 = ((ip[0] + ip[3]) << 5);
    b1 = ((ip[1] + ip[2]) << 5);
    c1 = ((ip[1] - ip[2]) << 5);
    d1 = ((ip[0] - ip[3]) << 5);

    op[0] = a1 + b1;
    op[2] = a1 - b1;

    op[1] = (c1 * 2217 + d1 * 5352 +  14500) >> 12;
    op[3] = (d1 * 2217 - c1 * 5352 +   7500) >> 12;

    ip += pitch / 2;
    op += 4;

  }
  ip = output;
  op = output;
  for (i = 0; i < 4; i++) {
    a1 = ip[0] + ip[12];
    b1 = ip[4] + ip[8];
    c1 = ip[4] - ip[8];
    d1 = ip[0] - ip[12];

    op[0]  = (a1 + b1 + 7) >> 4;
    op[8]  = (a1 - b1 + 7) >> 4;

    op[4]  = ((c1 * 2217 + d1 * 5352 +  12000) >> 16) + (d1 != 0);
    op[12] = (d1 * 2217 - c1 * 5352 +  51000) >> 16;

    ip++;
    op++;
  }
}

void vp9_short_fdct8x4_c(short *input, short *output, int pitch)
{
    vp9_short_fdct4x4_c(input,   output,    pitch);
    vp9_short_fdct4x4_c(input + 4, output + 16, pitch);
}

void vp9_short_walsh4x4_c(short *input, short *output, int pitch) {
  int i;
  int a1, b1, c1, d1;
  short *ip = input;
  short *op = output;
  int pitch_short = pitch >> 1;

  for (i = 0; i < 4; i++) {
    a1 = ip[0 * pitch_short] + ip[3 * pitch_short];
    b1 = ip[1 * pitch_short] + ip[2 * pitch_short];
    c1 = ip[1 * pitch_short] - ip[2 * pitch_short];
    d1 = ip[0 * pitch_short] - ip[3 * pitch_short];

    op[0] = (a1 + b1 + 1) >> 1;
    op[4] = (c1 + d1) >> 1;
    op[8] = (a1 - b1) >> 1;
    op[12] = (d1 - c1) >> 1;

    ip++;
    op++;
  }
  ip = output;
  op = output;

  for (i = 0; i < 4; i++) {
    a1 = ip[0] + ip[3];
    b1 = ip[1] + ip[2];
    c1 = ip[1] - ip[2];
    d1 = ip[0] - ip[3];

    op[0] = (a1 + b1 + 1) >> 1;
    op[1] = (c1 + d1) >> 1;
    op[2] = (a1 - b1) >> 1;
    op[3] = (d1 - c1) >> 1;

    ip += 4;
    op += 4;
  }
}

#if CONFIG_LOSSLESS
void vp9_short_walsh4x4_lossless_c(short *input, short *output, int pitch) {
  int i;
  int a1, b1, c1, d1;
  short *ip = input;
  short *op = output;
  int pitch_short = pitch >> 1;

  for (i = 0; i < 4; i++) {
    a1 = (ip[0 * pitch_short] + ip[3 * pitch_short]) >> Y2_WHT_UPSCALE_FACTOR;
    b1 = (ip[1 * pitch_short] + ip[2 * pitch_short]) >> Y2_WHT_UPSCALE_FACTOR;
    c1 = (ip[1 * pitch_short] - ip[2 * pitch_short]) >> Y2_WHT_UPSCALE_FACTOR;
    d1 = (ip[0 * pitch_short] - ip[3 * pitch_short]) >> Y2_WHT_UPSCALE_FACTOR;

    op[0] = (a1 + b1 + 1) >> 1;
    op[4] = (c1 + d1) >> 1;
    op[8] = (a1 - b1) >> 1;
    op[12] = (d1 - c1) >> 1;

    ip++;
    op++;
  }
  ip = output;
  op = output;

  for (i = 0; i < 4; i++) {
    a1 = ip[0] + ip[3];
    b1 = ip[1] + ip[2];
    c1 = ip[1] - ip[2];
    d1 = ip[0] - ip[3];

    op[0] = ((a1 + b1 + 1) >> 1) << Y2_WHT_UPSCALE_FACTOR;
    op[1] = ((c1 + d1) >> 1) << Y2_WHT_UPSCALE_FACTOR;
    op[2] = ((a1 - b1) >> 1) << Y2_WHT_UPSCALE_FACTOR;
    op[3] = ((d1 - c1) >> 1) << Y2_WHT_UPSCALE_FACTOR;

    ip += 4;
    op += 4;
  }
}

void vp9_short_walsh4x4_x8_c(short *input, short *output, int pitch) {
  int i;
  int a1, b1, c1, d1;
  short *ip = input;
  short *op = output;
  int pitch_short = pitch >> 1;

  for (i = 0; i < 4; i++) {
    a1 = ip[0 * pitch_short] + ip[3 * pitch_short];
    b1 = ip[1 * pitch_short] + ip[2 * pitch_short];
    c1 = ip[1 * pitch_short] - ip[2 * pitch_short];
    d1 = ip[0 * pitch_short] - ip[3 * pitch_short];

    op[0] = (a1 + b1 + 1) >> 1;
    op[4] = (c1 + d1) >> 1;
    op[8] = (a1 - b1) >> 1;
    op[12] = (d1 - c1) >> 1;

    ip++;
    op++;
  }
  ip = output;
  op = output;

  for (i = 0; i < 4; i++) {
    a1 = ip[0] + ip[3];
    b1 = ip[1] + ip[2];
    c1 = ip[1] - ip[2];
    d1 = ip[0] - ip[3];

    op[0] = ((a1 + b1 + 1) >> 1) << WHT_UPSCALE_FACTOR;
    op[1] = ((c1 + d1) >> 1) << WHT_UPSCALE_FACTOR;
    op[2] = ((a1 - b1) >> 1) << WHT_UPSCALE_FACTOR;
    op[3] = ((d1 - c1) >> 1) << WHT_UPSCALE_FACTOR;

    ip += 4;
    op += 4;
  }
}

void vp9_short_walsh8x4_x8_c(short *input, short *output, int pitch) {
  vp9_short_walsh4x4_x8_c(input,   output,    pitch);
  vp9_short_walsh4x4_x8_c(input + 4, output + 16, pitch);
}
#endif

#define TEST_INT_16x16_DCT 1
#if !TEST_INT_16x16_DCT

static void dct16x16_1d(double input[16], double output[16]) {
  static const double C1 = 0.995184726672197;
  static const double C2 = 0.98078528040323;
  static const double C3 = 0.956940335732209;
  static const double C4 = 0.923879532511287;
  static const double C5 = 0.881921264348355;
  static const double C6 = 0.831469612302545;
  static const double C7 = 0.773010453362737;
  static const double C8 = 0.707106781186548;
  static const double C9 = 0.634393284163646;
  static const double C10 = 0.555570233019602;
  static const double C11 = 0.471396736825998;
  static const double C12 = 0.38268343236509;
  static const double C13 = 0.290284677254462;
  static const double C14 = 0.195090322016128;
  static const double C15 = 0.098017140329561;

  vp9_clear_system_state(); // Make it simd safe : __asm emms;
  {
    double step[16];
    double intermediate[16];
    double temp1, temp2;

    // step 1
    step[ 0] = input[0] + input[15];
    step[ 1] = input[1] + input[14];
    step[ 2] = input[2] + input[13];
    step[ 3] = input[3] + input[12];
    step[ 4] = input[4] + input[11];
    step[ 5] = input[5] + input[10];
    step[ 6] = input[6] + input[ 9];
    step[ 7] = input[7] + input[ 8];
    step[ 8] = input[7] - input[ 8];
    step[ 9] = input[6] - input[ 9];
    step[10] = input[5] - input[10];
    step[11] = input[4] - input[11];
    step[12] = input[3] - input[12];
    step[13] = input[2] - input[13];
    step[14] = input[1] - input[14];
    step[15] = input[0] - input[15];

    // step 2
    output[0] = step[0] + step[7];
    output[1] = step[1] + step[6];
    output[2] = step[2] + step[5];
    output[3] = step[3] + step[4];
    output[4] = step[3] - step[4];
    output[5] = step[2] - step[5];
    output[6] = step[1] - step[6];
    output[7] = step[0] - step[7];

    temp1 = step[ 8]*C7;
    temp2 = step[15]*C9;
    output[ 8] = temp1 + temp2;

    temp1 = step[ 9]*C11;
    temp2 = step[14]*C5;
    output[ 9] = temp1 - temp2;

    temp1 = step[10]*C3;
    temp2 = step[13]*C13;
    output[10] = temp1 + temp2;

    temp1 = step[11]*C15;
    temp2 = step[12]*C1;
    output[11] = temp1 - temp2;

    temp1 = step[11]*C1;
    temp2 = step[12]*C15;
    output[12] = temp2 + temp1;

    temp1 = step[10]*C13;
    temp2 = step[13]*C3;
    output[13] = temp2 - temp1;

    temp1 = step[ 9]*C5;
    temp2 = step[14]*C11;
    output[14] = temp2 + temp1;

    temp1 = step[ 8]*C9;
    temp2 = step[15]*C7;
    output[15] = temp2 - temp1;

    // step 3
    step[ 0] = output[0] + output[3];
    step[ 1] = output[1] + output[2];
    step[ 2] = output[1] - output[2];
    step[ 3] = output[0] - output[3];

    temp1 = output[4]*C14;
    temp2 = output[7]*C2;
    step[ 4] = temp1 + temp2;

    temp1 = output[5]*C10;
    temp2 = output[6]*C6;
    step[ 5] = temp1 + temp2;

    temp1 = output[5]*C6;
    temp2 = output[6]*C10;
    step[ 6] = temp2 - temp1;

    temp1 = output[4]*C2;
    temp2 = output[7]*C14;
    step[ 7] = temp2 - temp1;

    step[ 8] = output[ 8] + output[11];
    step[ 9] = output[ 9] + output[10];
    step[10] = output[ 9] - output[10];
    step[11] = output[ 8] - output[11];

    step[12] = output[12] + output[15];
    step[13] = output[13] + output[14];
    step[14] = output[13] - output[14];
    step[15] = output[12] - output[15];

    // step 4
    output[ 0] = (step[ 0] + step[ 1]);
    output[ 8] = (step[ 0] - step[ 1]);

    temp1 = step[2]*C12;
    temp2 = step[3]*C4;
    temp1 = temp1 + temp2;
    output[ 4] = 2*(temp1*C8);

    temp1 = step[2]*C4;
    temp2 = step[3]*C12;
    temp1 = temp2 - temp1;
    output[12] = 2*(temp1*C8);

    output[ 2] = 2*((step[4] + step[ 5])*C8);
    output[14] = 2*((step[7] - step[ 6])*C8);

    temp1 = step[4] - step[5];
    temp2 = step[6] + step[7];
    output[ 6] = (temp1 + temp2);
    output[10] = (temp1 - temp2);

    intermediate[8] = step[8] + step[14];
    intermediate[9] = step[9] + step[15];

    temp1 = intermediate[8]*C12;
    temp2 = intermediate[9]*C4;
    temp1 = temp1 - temp2;
    output[3] = 2*(temp1*C8);

    temp1 = intermediate[8]*C4;
    temp2 = intermediate[9]*C12;
    temp1 = temp2 + temp1;
    output[13] = 2*(temp1*C8);

    output[ 9] = 2*((step[10] + step[11])*C8);

    intermediate[11] = step[10] - step[11];
    intermediate[12] = step[12] + step[13];
    intermediate[13] = step[12] - step[13];
    intermediate[14] = step[ 8] - step[14];
    intermediate[15] = step[ 9] - step[15];

    output[15] = (intermediate[11] + intermediate[12]);
    output[ 1] = -(intermediate[11] - intermediate[12]);

    output[ 7] = 2*(intermediate[13]*C8);

    temp1 = intermediate[14]*C12;
    temp2 = intermediate[15]*C4;
    temp1 = temp1 - temp2;
    output[11] = -2*(temp1*C8);

    temp1 = intermediate[14]*C4;
    temp2 = intermediate[15]*C12;
    temp1 = temp2 + temp1;
    output[ 5] = 2*(temp1*C8);
  }
  vp9_clear_system_state(); // Make it simd safe : __asm emms;
}

void vp9_short_fdct16x16_c(short *input, short *out, int pitch) {
  vp9_clear_system_state(); // Make it simd safe : __asm emms;
  {
    int shortpitch = pitch >> 1;
    int i, j;
    double output[256];
    // First transform columns
    for (i = 0; i < 16; i++) {
        double temp_in[16], temp_out[16];
        for (j = 0; j < 16; j++)
            temp_in[j] = input[j*shortpitch + i];
        dct16x16_1d(temp_in, temp_out);
        for (j = 0; j < 16; j++)
            output[j*16 + i] = temp_out[j];
    }
    // Then transform rows
    for (i = 0; i < 16; ++i) {
        double temp_in[16], temp_out[16];
        for (j = 0; j < 16; ++j)
            temp_in[j] = output[j + i*16];
        dct16x16_1d(temp_in, temp_out);
        for (j = 0; j < 16; ++j)
            output[j + i*16] = temp_out[j];
    }
    // Scale by some magic number
    for (i = 0; i < 256; i++)
        out[i] = (short)round(output[i]/2);
  }
  vp9_clear_system_state(); // Make it simd safe : __asm emms;
}

#else
static const int16_t C1 = 16305;
static const int16_t C2 = 16069;
static const int16_t C3 = 15679;
static const int16_t C4 = 15137;
static const int16_t C5 = 14449;
static const int16_t C6 = 13623;
static const int16_t C7 = 12665;
static const int16_t C8 = 11585;
static const int16_t C9 = 10394;
static const int16_t C10 = 9102;
static const int16_t C11 = 7723;
static const int16_t C12 = 6270;
static const int16_t C13 = 4756;
static const int16_t C14 = 3196;
static const int16_t C15 = 1606;

#define RIGHT_SHIFT 14
#define ROUNDING (1 << (RIGHT_SHIFT - 1))

static void dct16x16_1d(int16_t input[16], int16_t output[16],
                        int last_shift_bits) {
    int16_t step[16];
    int intermediate[16];
    int temp1, temp2;
    int final_shift = RIGHT_SHIFT;
    int final_rounding = ROUNDING;
    int output_shift = 0;
    int output_rounding = 0;

    final_shift += last_shift_bits;
    if (final_shift > 0)
    final_rounding = 1 << (final_shift - 1);

    output_shift += last_shift_bits;
    if (output_shift > 0)
      output_rounding = 1 << (output_shift - 1);

    // step 1
    step[ 0] = input[0] + input[15];
    step[ 1] = input[1] + input[14];
    step[ 2] = input[2] + input[13];
    step[ 3] = input[3] + input[12];
    step[ 4] = input[4] + input[11];
    step[ 5] = input[5] + input[10];
    step[ 6] = input[6] + input[ 9];
    step[ 7] = input[7] + input[ 8];
    step[ 8] = input[7] - input[ 8];
    step[ 9] = input[6] - input[ 9];
    step[10] = input[5] - input[10];
    step[11] = input[4] - input[11];
    step[12] = input[3] - input[12];
    step[13] = input[2] - input[13];
    step[14] = input[1] - input[14];
    step[15] = input[0] - input[15];

    // step 2
    output[0] = step[0] + step[7];
    output[1] = step[1] + step[6];
    output[2] = step[2] + step[5];
    output[3] = step[3] + step[4];
    output[4] = step[3] - step[4];
    output[5] = step[2] - step[5];
    output[6] = step[1] - step[6];
    output[7] = step[0] - step[7];

    temp1 = step[ 8] * C7;
    temp2 = step[15] * C9;
    output[ 8] = (temp1 + temp2 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[ 9] * C11;
    temp2 = step[14] * C5;
    output[ 9] = (temp1 - temp2 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[10] * C3;
    temp2 = step[13] * C13;
    output[10] = (temp1 + temp2 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[11] * C15;
    temp2 = step[12] * C1;
    output[11] = (temp1 - temp2 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[11] * C1;
    temp2 = step[12] * C15;
    output[12] = (temp2 + temp1 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[10] * C13;
    temp2 = step[13] * C3;
    output[13] = (temp2 - temp1 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[ 9] * C5;
    temp2 = step[14] * C11;
    output[14] = (temp2 + temp1 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[ 8] * C9;
    temp2 = step[15] * C7;
    output[15] = (temp2 - temp1 + ROUNDING) >> RIGHT_SHIFT;

    // step 3
    step[ 0] = output[0] + output[3];
    step[ 1] = output[1] + output[2];
    step[ 2] = output[1] - output[2];
    step[ 3] = output[0] - output[3];

    temp1 = output[4] * C14;
    temp2 = output[7] * C2;
    step[ 4] = (temp1 + temp2 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = output[5] * C10;
    temp2 = output[6] * C6;
    step[ 5] = (temp1 + temp2 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = output[5] * C6;
    temp2 = output[6] * C10;
    step[ 6] = (temp2 - temp1 + ROUNDING) >> RIGHT_SHIFT;

    temp1 = output[4] * C2;
    temp2 = output[7] * C14;
    step[ 7] = (temp2 - temp1 + ROUNDING) >> RIGHT_SHIFT;

    step[ 8] = output[ 8] + output[11];
    step[ 9] = output[ 9] + output[10];
    step[10] = output[ 9] - output[10];
    step[11] = output[ 8] - output[11];

    step[12] = output[12] + output[15];
    step[13] = output[13] + output[14];
    step[14] = output[13] - output[14];
    step[15] = output[12] - output[15];

    // step 4
    output[ 0] = (step[ 0] + step[ 1] + output_rounding) >> output_shift;
    output[ 8] = (step[ 0] - step[ 1] + output_rounding) >> output_shift;

    temp1 = step[2] * C12;
    temp2 = step[3] * C4;
    temp1 = (temp1 + temp2 + final_rounding) >> final_shift;
    output[ 4] = (2 * (temp1 * C8) + ROUNDING) >> RIGHT_SHIFT;

    temp1 = step[2] * C4;
    temp2 = step[3] * C12;
    temp1 = (temp2 - temp1 + final_rounding) >> final_shift;
    output[12] = (2 * (temp1 * C8) + ROUNDING) >> RIGHT_SHIFT;

    output[ 2] = (2 * ((step[4] + step[ 5]) * C8) + final_rounding)
        >> final_shift;
    output[14] = (2 * ((step[7] - step[ 6]) * C8) + final_rounding)
        >> final_shift;

    temp1 = step[4] - step[5];
    temp2 = step[6] + step[7];
    output[ 6] = (temp1 + temp2 + output_rounding) >> output_shift;
    output[10] = (temp1 - temp2 + output_rounding) >> output_shift;

    intermediate[8] = step[8] + step[14];
    intermediate[9] = step[9] + step[15];

    temp1 = intermediate[8] * C12;
    temp2 = intermediate[9] * C4;
    temp1 = (temp1 - temp2 + final_rounding) >> final_shift;
    output[3] = (2 * (temp1 * C8) + ROUNDING) >> RIGHT_SHIFT;

    temp1 = intermediate[8] * C4;
    temp2 = intermediate[9] * C12;
    temp1 = (temp2 + temp1 + final_rounding) >> final_shift;
    output[13] = (2 * (temp1 * C8) + ROUNDING) >> RIGHT_SHIFT;

    output[ 9] = (2 * ((step[10] + step[11]) * C8) + final_rounding)
        >> final_shift;

    intermediate[11] = step[10] - step[11];
    intermediate[12] = step[12] + step[13];
    intermediate[13] = step[12] - step[13];
    intermediate[14] = step[ 8] - step[14];
    intermediate[15] = step[ 9] - step[15];

    output[15] = (intermediate[11] + intermediate[12] + output_rounding)
        >> output_shift;
    output[ 1] = -(intermediate[11] - intermediate[12] + output_rounding)
        >> output_shift;

    output[ 7] = (2 * (intermediate[13] * C8) + final_rounding) >> final_shift;

    temp1 = intermediate[14] * C12;
    temp2 = intermediate[15] * C4;
    temp1 = (temp1 - temp2 + final_rounding) >> final_shift;
    output[11] = (-2 * (temp1 * C8) + ROUNDING) >> RIGHT_SHIFT;

    temp1 = intermediate[14] * C4;
    temp2 = intermediate[15] * C12;
    temp1 = (temp2 + temp1 + final_rounding) >> final_shift;
    output[ 5] = (2 * (temp1 * C8) + ROUNDING) >> RIGHT_SHIFT;
}

void vp9_short_fdct16x16_c(int16_t *input, int16_t *out, int pitch) {
    int shortpitch = pitch >> 1;
    int i, j;
    int16_t output[256];
    int16_t *outptr = &output[0];

    // First transform columns
    for (i = 0; i < 16; i++) {
        int16_t temp_in[16];
        int16_t temp_out[16];
        for (j = 0; j < 16; j++)
            temp_in[j] = input[j * shortpitch + i];
        dct16x16_1d(temp_in, temp_out, 0);
        for (j = 0; j < 16; j++)
            output[j * 16 + i] = temp_out[j];
    }

    // Then transform rows
    for (i = 0; i < 16; ++i) {
        dct16x16_1d(outptr, out, 1);
        outptr += 16;
        out += 16;
    }
}
#undef RIGHT_SHIFT
#undef ROUNDING
#endif

#if !CONFIG_DWTDCTHYBRID
static void dct32_1d(double *input, double *output, int stride) {
  static const double C1 = 0.998795456205;  // cos(pi * 1 / 64)
  static const double C2 = 0.995184726672;  // cos(pi * 2 / 64)
  static const double C3 = 0.989176509965;  // cos(pi * 3 / 64)
  static const double C4 = 0.980785280403;  // cos(pi * 4 / 64)
  static const double C5 = 0.970031253195;  // cos(pi * 5 / 64)
  static const double C6 = 0.956940335732;  // cos(pi * 6 / 64)
  static const double C7 = 0.941544065183;  // cos(pi * 7 / 64)
  static const double C8 = 0.923879532511;  // cos(pi * 8 / 64)
  static const double C9 = 0.903989293123;  // cos(pi * 9 / 64)
  static const double C10 = 0.881921264348;  // cos(pi * 10 / 64)
  static const double C11 = 0.857728610000;  // cos(pi * 11 / 64)
  static const double C12 = 0.831469612303;  // cos(pi * 12 / 64)
  static const double C13 = 0.803207531481;  // cos(pi * 13 / 64)
  static const double C14 = 0.773010453363;  // cos(pi * 14 / 64)
  static const double C15 = 0.740951125355;  // cos(pi * 15 / 64)
  static const double C16 = 0.707106781187;  // cos(pi * 16 / 64)
  static const double C17 = 0.671558954847;  // cos(pi * 17 / 64)
  static const double C18 = 0.634393284164;  // cos(pi * 18 / 64)
  static const double C19 = 0.595699304492;  // cos(pi * 19 / 64)
  static const double C20 = 0.555570233020;  // cos(pi * 20 / 64)
  static const double C21 = 0.514102744193;  // cos(pi * 21 / 64)
  static const double C22 = 0.471396736826;  // cos(pi * 22 / 64)
  static const double C23 = 0.427555093430;  // cos(pi * 23 / 64)
  static const double C24 = 0.382683432365;  // cos(pi * 24 / 64)
  static const double C25 = 0.336889853392;  // cos(pi * 25 / 64)
  static const double C26 = 0.290284677254;  // cos(pi * 26 / 64)
  static const double C27 = 0.242980179903;  // cos(pi * 27 / 64)
  static const double C28 = 0.195090322016;  // cos(pi * 28 / 64)
  static const double C29 = 0.146730474455;  // cos(pi * 29 / 64)
  static const double C30 = 0.098017140330;  // cos(pi * 30 / 64)
  static const double C31 = 0.049067674327;  // cos(pi * 31 / 64)

  double step[32];

  // Stage 1
  step[0] = input[stride*0] + input[stride*(32 - 1)];
  step[1] = input[stride*1] + input[stride*(32 - 2)];
  step[2] = input[stride*2] + input[stride*(32 - 3)];
  step[3] = input[stride*3] + input[stride*(32 - 4)];
  step[4] = input[stride*4] + input[stride*(32 - 5)];
  step[5] = input[stride*5] + input[stride*(32 - 6)];
  step[6] = input[stride*6] + input[stride*(32 - 7)];
  step[7] = input[stride*7] + input[stride*(32 - 8)];
  step[8] = input[stride*8] + input[stride*(32 - 9)];
  step[9] = input[stride*9] + input[stride*(32 - 10)];
  step[10] = input[stride*10] + input[stride*(32 - 11)];
  step[11] = input[stride*11] + input[stride*(32 - 12)];
  step[12] = input[stride*12] + input[stride*(32 - 13)];
  step[13] = input[stride*13] + input[stride*(32 - 14)];
  step[14] = input[stride*14] + input[stride*(32 - 15)];
  step[15] = input[stride*15] + input[stride*(32 - 16)];
  step[16] = -input[stride*16] + input[stride*(32 - 17)];
  step[17] = -input[stride*17] + input[stride*(32 - 18)];
  step[18] = -input[stride*18] + input[stride*(32 - 19)];
  step[19] = -input[stride*19] + input[stride*(32 - 20)];
  step[20] = -input[stride*20] + input[stride*(32 - 21)];
  step[21] = -input[stride*21] + input[stride*(32 - 22)];
  step[22] = -input[stride*22] + input[stride*(32 - 23)];
  step[23] = -input[stride*23] + input[stride*(32 - 24)];
  step[24] = -input[stride*24] + input[stride*(32 - 25)];
  step[25] = -input[stride*25] + input[stride*(32 - 26)];
  step[26] = -input[stride*26] + input[stride*(32 - 27)];
  step[27] = -input[stride*27] + input[stride*(32 - 28)];
  step[28] = -input[stride*28] + input[stride*(32 - 29)];
  step[29] = -input[stride*29] + input[stride*(32 - 30)];
  step[30] = -input[stride*30] + input[stride*(32 - 31)];
  step[31] = -input[stride*31] + input[stride*(32 - 32)];

  // Stage 2
  output[stride*0] = step[0] + step[16 - 1];
  output[stride*1] = step[1] + step[16 - 2];
  output[stride*2] = step[2] + step[16 - 3];
  output[stride*3] = step[3] + step[16 - 4];
  output[stride*4] = step[4] + step[16 - 5];
  output[stride*5] = step[5] + step[16 - 6];
  output[stride*6] = step[6] + step[16 - 7];
  output[stride*7] = step[7] + step[16 - 8];
  output[stride*8] = -step[8] + step[16 - 9];
  output[stride*9] = -step[9] + step[16 - 10];
  output[stride*10] = -step[10] + step[16 - 11];
  output[stride*11] = -step[11] + step[16 - 12];
  output[stride*12] = -step[12] + step[16 - 13];
  output[stride*13] = -step[13] + step[16 - 14];
  output[stride*14] = -step[14] + step[16 - 15];
  output[stride*15] = -step[15] + step[16 - 16];

  output[stride*16] = step[16];
  output[stride*17] = step[17];
  output[stride*18] = step[18];
  output[stride*19] = step[19];

  output[stride*20] = (-step[20] + step[27])*C16;
  output[stride*21] = (-step[21] + step[26])*C16;
  output[stride*22] = (-step[22] + step[25])*C16;
  output[stride*23] = (-step[23] + step[24])*C16;

  output[stride*24] = (step[24] + step[23])*C16;
  output[stride*25] = (step[25] + step[22])*C16;
  output[stride*26] = (step[26] + step[21])*C16;
  output[stride*27] = (step[27] + step[20])*C16;

  output[stride*28] = step[28];
  output[stride*29] = step[29];
  output[stride*30] = step[30];
  output[stride*31] = step[31];

  // Stage 3
  step[0] = output[stride*0] + output[stride*(8 - 1)];
  step[1] = output[stride*1] + output[stride*(8 - 2)];
  step[2] = output[stride*2] + output[stride*(8 - 3)];
  step[3] = output[stride*3] + output[stride*(8 - 4)];
  step[4] = -output[stride*4] + output[stride*(8 - 5)];
  step[5] = -output[stride*5] + output[stride*(8 - 6)];
  step[6] = -output[stride*6] + output[stride*(8 - 7)];
  step[7] = -output[stride*7] + output[stride*(8 - 8)];
  step[8] = output[stride*8];
  step[9] = output[stride*9];
  step[10] = (-output[stride*10] + output[stride*13])*C16;
  step[11] = (-output[stride*11] + output[stride*12])*C16;
  step[12] = (output[stride*12] + output[stride*11])*C16;
  step[13] = (output[stride*13] + output[stride*10])*C16;
  step[14] = output[stride*14];
  step[15] = output[stride*15];

  step[16] = output[stride*16] + output[stride*23];
  step[17] = output[stride*17] + output[stride*22];
  step[18] = output[stride*18] + output[stride*21];
  step[19] = output[stride*19] + output[stride*20];
  step[20] = -output[stride*20] + output[stride*19];
  step[21] = -output[stride*21] + output[stride*18];
  step[22] = -output[stride*22] + output[stride*17];
  step[23] = -output[stride*23] + output[stride*16];
  step[24] = -output[stride*24] + output[stride*31];
  step[25] = -output[stride*25] + output[stride*30];
  step[26] = -output[stride*26] + output[stride*29];
  step[27] = -output[stride*27] + output[stride*28];
  step[28] = output[stride*28] + output[stride*27];
  step[29] = output[stride*29] + output[stride*26];
  step[30] = output[stride*30] + output[stride*25];
  step[31] = output[stride*31] + output[stride*24];

  // Stage 4
  output[stride*0] = step[0] + step[3];
  output[stride*1] = step[1] + step[2];
  output[stride*2] = -step[2] + step[1];
  output[stride*3] = -step[3] + step[0];
  output[stride*4] = step[4];
  output[stride*5] = (-step[5] + step[6])*C16;
  output[stride*6] = (step[6] + step[5])*C16;
  output[stride*7] = step[7];
  output[stride*8] = step[8] + step[11];
  output[stride*9] = step[9] + step[10];
  output[stride*10] = -step[10] + step[9];
  output[stride*11] = -step[11] + step[8];
  output[stride*12] = -step[12] + step[15];
  output[stride*13] = -step[13] + step[14];
  output[stride*14] = step[14] + step[13];
  output[stride*15] = step[15] + step[12];

  output[stride*16] = step[16];
  output[stride*17] = step[17];
  output[stride*18] = step[18]*-C8 + step[29]*C24;
  output[stride*19] = step[19]*-C8 + step[28]*C24;
  output[stride*20] = step[20]*-C24 + step[27]*-C8;
  output[stride*21] = step[21]*-C24 + step[26]*-C8;
  output[stride*22] = step[22];
  output[stride*23] = step[23];
  output[stride*24] = step[24];
  output[stride*25] = step[25];
  output[stride*26] = step[26]*C24 + step[21]*-C8;
  output[stride*27] = step[27]*C24 + step[20]*-C8;
  output[stride*28] = step[28]*C8 + step[19]*C24;
  output[stride*29] = step[29]*C8 + step[18]*C24;
  output[stride*30] = step[30];
  output[stride*31] = step[31];

  // Stage 5
  step[0] = (output[stride*0] + output[stride*1]) * C16;
  step[1] = (-output[stride*1] + output[stride*0]) * C16;
  step[2] = output[stride*2]*C24 + output[stride*3] * C8;
  step[3] = output[stride*3]*C24 - output[stride*2] * C8;
  step[4] = output[stride*4] + output[stride*5];
  step[5] = -output[stride*5] + output[stride*4];
  step[6] = -output[stride*6] + output[stride*7];
  step[7] = output[stride*7] + output[stride*6];
  step[8] = output[stride*8];
  step[9] = output[stride*9]*-C8 + output[stride*14]*C24;
  step[10] = output[stride*10]*-C24 + output[stride*13]*-C8;
  step[11] = output[stride*11];
  step[12] = output[stride*12];
  step[13] = output[stride*13]*C24 + output[stride*10]*-C8;
  step[14] = output[stride*14]*C8 + output[stride*9]*C24;
  step[15] = output[stride*15];

  step[16] = output[stride*16] + output[stride*19];
  step[17] = output[stride*17] + output[stride*18];
  step[18] = -output[stride*18] + output[stride*17];
  step[19] = -output[stride*19] + output[stride*16];
  step[20] = -output[stride*20] + output[stride*23];
  step[21] = -output[stride*21] + output[stride*22];
  step[22] = output[stride*22] + output[stride*21];
  step[23] = output[stride*23] + output[stride*20];
  step[24] = output[stride*24] + output[stride*27];
  step[25] = output[stride*25] + output[stride*26];
  step[26] = -output[stride*26] + output[stride*25];
  step[27] = -output[stride*27] + output[stride*24];
  step[28] = -output[stride*28] + output[stride*31];
  step[29] = -output[stride*29] + output[stride*30];
  step[30] = output[stride*30] + output[stride*29];
  step[31] = output[stride*31] + output[stride*28];

  // Stage 6
  output[stride*0] = step[0];
  output[stride*1] = step[1];
  output[stride*2] = step[2];
  output[stride*3] = step[3];
  output[stride*4] = step[4]*C28 + step[7]*C4;
  output[stride*5] = step[5]*C12 + step[6]*C20;
  output[stride*6] = step[6]*C12 + step[5]*-C20;
  output[stride*7] = step[7]*C28 + step[4]*-C4;
  output[stride*8] = step[8] + step[9];
  output[stride*9] = -step[9] + step[8];
  output[stride*10] = -step[10] + step[11];
  output[stride*11] = step[11] + step[10];
  output[stride*12] = step[12] + step[13];
  output[stride*13] = -step[13] + step[12];
  output[stride*14] = -step[14] + step[15];
  output[stride*15] = step[15] + step[14];

  output[stride*16] = step[16];
  output[stride*17] = step[17]*-C4 + step[30]*C28;
  output[stride*18] = step[18]*-C28 + step[29]*-C4;
  output[stride*19] = step[19];
  output[stride*20] = step[20];
  output[stride*21] = step[21]*-C20 + step[26]*C12;
  output[stride*22] = step[22]*-C12 + step[25]*-C20;
  output[stride*23] = step[23];
  output[stride*24] = step[24];
  output[stride*25] = step[25]*C12 + step[22]*-C20;
  output[stride*26] = step[26]*C20 + step[21]*C12;
  output[stride*27] = step[27];
  output[stride*28] = step[28];
  output[stride*29] = step[29]*C28 + step[18]*-C4;
  output[stride*30] = step[30]*C4 + step[17]*C28;
  output[stride*31] = step[31];

  // Stage 7
  step[0] = output[stride*0];
  step[1] = output[stride*1];
  step[2] = output[stride*2];
  step[3] = output[stride*3];
  step[4] = output[stride*4];
  step[5] = output[stride*5];
  step[6] = output[stride*6];
  step[7] = output[stride*7];
  step[8] = output[stride*8]*C30 + output[stride*15]*C2;
  step[9] = output[stride*9]*C14 + output[stride*14]*C18;
  step[10] = output[stride*10]*C22 + output[stride*13]*C10;
  step[11] = output[stride*11]*C6 + output[stride*12]*C26;
  step[12] = output[stride*12]*C6 + output[stride*11]*-C26;
  step[13] = output[stride*13]*C22 + output[stride*10]*-C10;
  step[14] = output[stride*14]*C14 + output[stride*9]*-C18;
  step[15] = output[stride*15]*C30 + output[stride*8]*-C2;

  step[16] = output[stride*16] + output[stride*17];
  step[17] = -output[stride*17] + output[stride*16];
  step[18] = -output[stride*18] + output[stride*19];
  step[19] = output[stride*19] + output[stride*18];
  step[20] = output[stride*20] + output[stride*21];
  step[21] = -output[stride*21] + output[stride*20];
  step[22] = -output[stride*22] + output[stride*23];
  step[23] = output[stride*23] + output[stride*22];
  step[24] = output[stride*24] + output[stride*25];
  step[25] = -output[stride*25] + output[stride*24];
  step[26] = -output[stride*26] + output[stride*27];
  step[27] = output[stride*27] + output[stride*26];
  step[28] = output[stride*28] + output[stride*29];
  step[29] = -output[stride*29] + output[stride*28];
  step[30] = -output[stride*30] + output[stride*31];
  step[31] = output[stride*31] + output[stride*30];

  // Final stage --- outputs indices are bit-reversed.
  output[stride*0] = step[0];
  output[stride*16] = step[1];
  output[stride*8] = step[2];
  output[stride*24] = step[3];
  output[stride*4] = step[4];
  output[stride*20] = step[5];
  output[stride*12] = step[6];
  output[stride*28] = step[7];
  output[stride*2] = step[8];
  output[stride*18] = step[9];
  output[stride*10] = step[10];
  output[stride*26] = step[11];
  output[stride*6] = step[12];
  output[stride*22] = step[13];
  output[stride*14] = step[14];
  output[stride*30] = step[15];

  output[stride*1] = step[16]*C31 + step[31]*C1;
  output[stride*17] = step[17]*C15 + step[30]*C17;
  output[stride*9] = step[18]*C23 + step[29]*C9;
  output[stride*25] = step[19]*C7 + step[28]*C25;
  output[stride*5] = step[20]*C27 + step[27]*C5;
  output[stride*21] = step[21]*C11 + step[26]*C21;
  output[stride*13] = step[22]*C19 + step[25]*C13;
  output[stride*29] = step[23]*C3 + step[24]*C29;
  output[stride*3] = step[24]*C3 + step[23]*-C29;
  output[stride*19] = step[25]*C19 + step[22]*-C13;
  output[stride*11] = step[26]*C11 + step[21]*-C21;
  output[stride*27] = step[27]*C27 + step[20]*-C5;
  output[stride*7] = step[28]*C7 + step[19]*-C25;
  output[stride*23] = step[29]*C23 + step[18]*-C9;
  output[stride*15] = step[30]*C15 + step[17]*-C17;
  output[stride*31] = step[31]*C31 + step[16]*-C1;
}

void vp9_short_fdct32x32_c(int16_t *input, int16_t *out, int pitch) {
  vp9_clear_system_state();  // Make it simd safe : __asm emms;
  {
    int shortpitch = pitch >> 1;
    int i, j;
    double output[1024];
    // First transform columns
    for (i = 0; i < 32; i++) {
      double temp_in[32], temp_out[32];
      for (j = 0; j < 32; j++)
        temp_in[j] = input[j*shortpitch + i];
      dct32_1d(temp_in, temp_out, 1);
      for (j = 0; j < 32; j++)
        output[j*32 + i] = temp_out[j];
    }
    // Then transform rows
    for (i = 0; i < 32; ++i) {
      double temp_in[32], temp_out[32];
      for (j = 0; j < 32; ++j)
        temp_in[j] = output[j + i*32];
      dct32_1d(temp_in, temp_out, 1);
      for (j = 0; j < 32; ++j)
        output[j + i*32] = temp_out[j];
    }
    // Scale by some magic number
    for (i = 0; i < 1024; i++) {
      out[i] = (short)round(output[i]/4);
    }
  }

  vp9_clear_system_state();  // Make it simd safe : __asm emms;
}

#else  // CONFIG_DWTDCTHYBRID

#if DWT_TYPE == 53

// Note: block length must be even for this implementation
static void analysis_53_row(int length, short *x,
                            short *lowpass, short *highpass) {
  int n;
  short r, *a, *b;

  n = length >> 1;
  b = highpass;
  a = lowpass;
  while (--n) {
    *a++ = (r = *x++) << 1;
    *b++ = *x - ((r + x[1] + 1) >> 1);
    x++;
  }
  *a = (r = *x++) << 1;
  *b = *x - r;

  n = length >> 1;
  b = highpass;
  a = lowpass;
  r = *highpass;
  while (n--) {
    *a++ += (r + (*b) + 1) >> 1;
    r = *b++;
  }
}

static void analysis_53_col(int length, short *x,
                            short *lowpass, short *highpass) {
  int n;
  short r, *a, *b;

  n = length >> 1;
  b = highpass;
  a = lowpass;
  while (--n) {
    *a++ = (r = *x++);
    *b++ = (((*x) << 1) - (r + x[1]) + 2) >> 2;
    x++;
  }
  *a = (r = *x++);
  *b = (*x - r + 1) >> 1;

  n = length >> 1;
  b = highpass;
  a = lowpass;
  r = *highpass;
  while (n--) {
    *a++ += (r + (*b) + 1) >> 1;
    r = *b++;
  }
}

static void dyadic_analyze_53(int levels, int width, int height,
                              short *x, int pitch_x, short *c, int pitch_c) {
  int lv, i, j, nh, nw, hh = height, hw = width;
  short buffer[2 * DWT_MAX_LENGTH];
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      c[i * pitch_c + j] = x[i * pitch_x + j] << DWT_PRECISION_BITS;
    }
  }
  for (lv = 0; lv < levels; lv++) {
    nh = hh;
    hh = (hh + 1) >> 1;
    nw = hw;
    hw = (hw + 1) >> 1;
    if ((nh < 2) || (nw < 2)) return;
    for (i = 0; i < nh; i++) {
      memcpy(buffer, &c[i * pitch_c], nw * sizeof(short));
      analysis_53_row(nw, buffer, &c[i * pitch_c], &c[i * pitch_c] + hw);
    }
    for (j = 0; j < nw; j++) {
      for (i = 0; i < nh; i++)
        buffer[i + nh] = c[i * pitch_c + j];
      analysis_53_col(nh, buffer + nh, buffer, buffer + hh);
      for (i = 0; i < nh; i++)
        c[i * pitch_c + j] = buffer[i];
    }
  }
}

#elif DWT_TYPE == 26

static void analysis_26_row(int length, short *x,
                            short *lowpass, short *highpass) {
  int i, n;
  short r, s, *a, *b;
  a = lowpass;
  b = highpass;
  for (i = length >> 1; i; i--) {
    r = *x++;
    s = *x++;
    *a++ = r + s;
    *b++ = r - s;
  }
  n = length >> 1;
  if (n >= 4) {
    a = lowpass;
    b = highpass;
    r = *lowpass;
    while (--n) {
      *b++ -= (r - a[1] + 4) >> 3;
      r = *a++;
    }
    *b -= (r - *a + 4) >> 3;
  }
}

static void analysis_26_col(int length, short *x,
                            short *lowpass, short *highpass) {
  int i, n;
  short r, s, *a, *b;
  a = lowpass;
  b = highpass;
  for (i = length >> 1; i; i--) {
    r = *x++;
    s = *x++;
    *a++ = (r + s + 1) >> 1;
    *b++ = (r - s + 1) >> 1;
  }
  n = length >> 1;
  if (n >= 4) {
    a = lowpass;
    b = highpass;
    r = *lowpass;
    while (--n) {
      *b++ -= (r - a[1] + 4) >> 3;
      r = *a++;
    }
    *b -= (r - *a + 4) >> 3;
  }
}

static void dyadic_analyze_26(int levels, int width, int height,
                              short *x, int pitch_x, short *c, int pitch_c) {
  int lv, i, j, nh, nw, hh = height, hw = width;
  short buffer[2 * DWT_MAX_LENGTH];
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      c[i * pitch_c + j] = x[i * pitch_x + j] << DWT_PRECISION_BITS;
    }
  }
  for (lv = 0; lv < levels; lv++) {
    nh = hh;
    hh = (hh + 1) >> 1;
    nw = hw;
    hw = (hw + 1) >> 1;
    if ((nh < 2) || (nw < 2)) return;
    for (i = 0; i < nh; i++) {
      memcpy(buffer, &c[i * pitch_c], nw * sizeof(short));
      analysis_26_row(nw, buffer, &c[i * pitch_c], &c[i * pitch_c] + hw);
    }
    for (j = 0; j < nw; j++) {
      for (i = 0; i < nh; i++)
        buffer[i + nh] = c[i * pitch_c + j];
      analysis_26_col(nh, buffer + nh, buffer, buffer + hh);
      for (i = 0; i < nh; i++)
        c[i * pitch_c + j] = buffer[i];
    }
  }
}

#elif DWT_TYPE == 97

static void analysis_97(int length, double *x,
                        double *lowpass, double *highpass) {
  static const double a_predict1 = -1.586134342;
  static const double a_update1 = -0.05298011854;
  static const double a_predict2 = 0.8829110762;
  static const double a_update2 = 0.4435068522;
  static const double s_low = 1.149604398;
  static const double s_high = 1/1.149604398;
  int i;
  double y[DWT_MAX_LENGTH];
  // Predict 1
  for (i = 1; i < length - 2; i += 2) {
    x[i] += a_predict1 * (x[i - 1] + x[i + 1]);
  }
  x[length - 1] += 2 * a_predict1 * x[length - 2];
  // Update 1
  for (i = 2; i < length; i += 2) {
    x[i] += a_update1 * (x[i - 1] + x[i + 1]);
  }
  x[0] += 2 * a_update1 * x[1];
  // Predict 2
  for (i = 1; i < length - 2; i += 2) {
    x[i] += a_predict2 * (x[i - 1] + x[i + 1]);
  }
  x[length - 1] += 2 * a_predict2 * x[length - 2];
  // Update 2
  for (i = 2; i < length; i += 2) {
    x[i] += a_update2 * (x[i - 1] + x[i + 1]);
  }
  x[0] += 2 * a_update2 * x[1];
  memcpy(y, x, sizeof(*y) * length);
  // Scale and pack
  for (i = 0; i < length / 2; i++) {
    lowpass[i] = y[2 * i] * s_low;
    highpass[i] = y[2 * i + 1] * s_high;
  }
}

static void dyadic_analyze_97(int levels, int width, int height,
                             short *x, int pitch_x, short *c, int pitch_c) {
  int lv, i, j, nh, nw, hh = height, hw = width;
  double buffer[2 * DWT_MAX_LENGTH];
  double y[DWT_MAX_LENGTH * DWT_MAX_LENGTH];
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      y[i * DWT_MAX_LENGTH + j] = x[i * pitch_x + j] << DWT_PRECISION_BITS;
    }
  }
  for (lv = 0; lv < levels; lv++) {
    nh = hh;
    hh = (hh + 1) >> 1;
    nw = hw;
    hw = (hw + 1) >> 1;
    if ((nh < 2) || (nw < 2)) return;
    for (i = 0; i < nh; i++) {
      memcpy(buffer, &y[i * DWT_MAX_LENGTH], nw * sizeof(*buffer));
      analysis_97(nw, buffer, &y[i * DWT_MAX_LENGTH],
                  &y[i * DWT_MAX_LENGTH] + hw);
    }
    for (j = 0; j < nw; j++) {
      for (i = 0; i < nh; i++)
        buffer[i + nh] = y[i * DWT_MAX_LENGTH + j];
      analysis_97(nh, buffer + nh, buffer, buffer + hh);
      for (i = 0; i < nh; i++)
        c[i * pitch_c + j] = round(buffer[i]);
    }
  }
}

#endif  // DWT_TYPE

// TODO(debargha): Implement the scaling differently so as not to have to
// use the floating point dct
static void dct16x16_1d_f(double input[16], double output[16]) {
  static const double C1 = 0.995184726672197;
  static const double C2 = 0.98078528040323;
  static const double C3 = 0.956940335732209;
  static const double C4 = 0.923879532511287;
  static const double C5 = 0.881921264348355;
  static const double C6 = 0.831469612302545;
  static const double C7 = 0.773010453362737;
  static const double C8 = 0.707106781186548;
  static const double C9 = 0.634393284163646;
  static const double C10 = 0.555570233019602;
  static const double C11 = 0.471396736825998;
  static const double C12 = 0.38268343236509;
  static const double C13 = 0.290284677254462;
  static const double C14 = 0.195090322016128;
  static const double C15 = 0.098017140329561;

  vp9_clear_system_state();  // Make it simd safe : __asm emms;
  {
    double step[16];
    double intermediate[16];
    double temp1, temp2;

    // step 1
    step[ 0] = input[0] + input[15];
    step[ 1] = input[1] + input[14];
    step[ 2] = input[2] + input[13];
    step[ 3] = input[3] + input[12];
    step[ 4] = input[4] + input[11];
    step[ 5] = input[5] + input[10];
    step[ 6] = input[6] + input[ 9];
    step[ 7] = input[7] + input[ 8];
    step[ 8] = input[7] - input[ 8];
    step[ 9] = input[6] - input[ 9];
    step[10] = input[5] - input[10];
    step[11] = input[4] - input[11];
    step[12] = input[3] - input[12];
    step[13] = input[2] - input[13];
    step[14] = input[1] - input[14];
    step[15] = input[0] - input[15];

    // step 2
    output[0] = step[0] + step[7];
    output[1] = step[1] + step[6];
    output[2] = step[2] + step[5];
    output[3] = step[3] + step[4];
    output[4] = step[3] - step[4];
    output[5] = step[2] - step[5];
    output[6] = step[1] - step[6];
    output[7] = step[0] - step[7];

    temp1 = step[ 8]*C7;
    temp2 = step[15]*C9;
    output[ 8] = temp1 + temp2;

    temp1 = step[ 9]*C11;
    temp2 = step[14]*C5;
    output[ 9] = temp1 - temp2;

    temp1 = step[10]*C3;
    temp2 = step[13]*C13;
    output[10] = temp1 + temp2;

    temp1 = step[11]*C15;
    temp2 = step[12]*C1;
    output[11] = temp1 - temp2;

    temp1 = step[11]*C1;
    temp2 = step[12]*C15;
    output[12] = temp2 + temp1;

    temp1 = step[10]*C13;
    temp2 = step[13]*C3;
    output[13] = temp2 - temp1;

    temp1 = step[ 9]*C5;
    temp2 = step[14]*C11;
    output[14] = temp2 + temp1;

    temp1 = step[ 8]*C9;
    temp2 = step[15]*C7;
    output[15] = temp2 - temp1;

    // step 3
    step[ 0] = output[0] + output[3];
    step[ 1] = output[1] + output[2];
    step[ 2] = output[1] - output[2];
    step[ 3] = output[0] - output[3];

    temp1 = output[4]*C14;
    temp2 = output[7]*C2;
    step[ 4] = temp1 + temp2;

    temp1 = output[5]*C10;
    temp2 = output[6]*C6;
    step[ 5] = temp1 + temp2;

    temp1 = output[5]*C6;
    temp2 = output[6]*C10;
    step[ 6] = temp2 - temp1;

    temp1 = output[4]*C2;
    temp2 = output[7]*C14;
    step[ 7] = temp2 - temp1;

    step[ 8] = output[ 8] + output[11];
    step[ 9] = output[ 9] + output[10];
    step[10] = output[ 9] - output[10];
    step[11] = output[ 8] - output[11];

    step[12] = output[12] + output[15];
    step[13] = output[13] + output[14];
    step[14] = output[13] - output[14];
    step[15] = output[12] - output[15];

    // step 4
    output[ 0] = (step[ 0] + step[ 1]);
    output[ 8] = (step[ 0] - step[ 1]);

    temp1 = step[2]*C12;
    temp2 = step[3]*C4;
    temp1 = temp1 + temp2;
    output[ 4] = 2*(temp1*C8);

    temp1 = step[2]*C4;
    temp2 = step[3]*C12;
    temp1 = temp2 - temp1;
    output[12] = 2*(temp1*C8);

    output[ 2] = 2*((step[4] + step[ 5])*C8);
    output[14] = 2*((step[7] - step[ 6])*C8);

    temp1 = step[4] - step[5];
    temp2 = step[6] + step[7];
    output[ 6] = (temp1 + temp2);
    output[10] = (temp1 - temp2);

    intermediate[8] = step[8] + step[14];
    intermediate[9] = step[9] + step[15];

    temp1 = intermediate[8]*C12;
    temp2 = intermediate[9]*C4;
    temp1 = temp1 - temp2;
    output[3] = 2*(temp1*C8);

    temp1 = intermediate[8]*C4;
    temp2 = intermediate[9]*C12;
    temp1 = temp2 + temp1;
    output[13] = 2*(temp1*C8);

    output[ 9] = 2*((step[10] + step[11])*C8);

    intermediate[11] = step[10] - step[11];
    intermediate[12] = step[12] + step[13];
    intermediate[13] = step[12] - step[13];
    intermediate[14] = step[ 8] - step[14];
    intermediate[15] = step[ 9] - step[15];

    output[15] = (intermediate[11] + intermediate[12]);
    output[ 1] = -(intermediate[11] - intermediate[12]);

    output[ 7] = 2*(intermediate[13]*C8);

    temp1 = intermediate[14]*C12;
    temp2 = intermediate[15]*C4;
    temp1 = temp1 - temp2;
    output[11] = -2*(temp1*C8);

    temp1 = intermediate[14]*C4;
    temp2 = intermediate[15]*C12;
    temp1 = temp2 + temp1;
    output[ 5] = 2*(temp1*C8);
  }
  vp9_clear_system_state();  // Make it simd safe : __asm emms;
}

static void vp9_short_fdct16x16_c_f(short *input, short *out, int pitch,
                                    int scale) {
  vp9_clear_system_state();  // Make it simd safe : __asm emms;
  {
    int shortpitch = pitch >> 1;
    int i, j;
    double output[256];
    // First transform columns
    for (i = 0; i < 16; i++) {
        double temp_in[16], temp_out[16];
        for (j = 0; j < 16; j++)
            temp_in[j] = input[j*shortpitch + i];
        dct16x16_1d_f(temp_in, temp_out);
        for (j = 0; j < 16; j++)
            output[j*16 + i] = temp_out[j];
    }
    // Then transform rows
    for (i = 0; i < 16; ++i) {
        double temp_in[16], temp_out[16];
        for (j = 0; j < 16; ++j)
            temp_in[j] = output[j + i*16];
        dct16x16_1d_f(temp_in, temp_out);
        for (j = 0; j < 16; ++j)
            output[j + i*16] = temp_out[j];
    }
    // Scale by some magic number
    for (i = 0; i < 256; i++)
        out[i] = (short)round(output[i] / (2 << scale));
  }
  vp9_clear_system_state();  // Make it simd safe : __asm emms;
}

void vp9_short_fdct8x8_c_f(short *block, short *coefs, int pitch, int scale) {
  int j1, i, j, k;
  float b[8];
  float b1[8];
  float d[8][8];
  float f0 = (float) .7071068;
  float f1 = (float) .4903926;
  float f2 = (float) .4619398;
  float f3 = (float) .4157348;
  float f4 = (float) .3535534;
  float f5 = (float) .2777851;
  float f6 = (float) .1913417;
  float f7 = (float) .0975452;
  pitch = pitch / 2;
  for (i = 0, k = 0; i < 8; i++, k += pitch) {
    for (j = 0; j < 8; j++) {
      b[j] = (float)(block[k + j] << (3 - scale));
    }
    /* Horizontal transform */
    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      b1[j] = b[j] + b[j1];
      b1[j1] = b[j] - b[j1];
    }
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];
    b[4] = b1[4];
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[7] = b1[7];
    d[i][0] = (b[0] + b[1]) * f4;
    d[i][4] = (b[0] - b[1]) * f4;
    d[i][2] = b[2] * f6 + b[3] * f2;
    d[i][6] = b[3] * f6 - b[2] * f2;
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[i][1] = b1[4] * f7 + b1[7] * f1;
    d[i][5] = b1[5] * f3 + b1[6] * f5;
    d[i][7] = b1[7] * f7 - b1[4] * f1;
    d[i][3] = b1[6] * f3 - b1[5] * f5;
  }
  /* Vertical transform */
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      b1[j] = d[j][i] + d[j1][i];
      b1[j1] = d[j][i] - d[j1][i];
    }
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];
    b[4] = b1[4];
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[7] = b1[7];
    d[0][i] = (b[0] + b[1]) * f4;
    d[4][i] = (b[0] - b[1]) * f4;
    d[2][i] = b[2] * f6 + b[3] * f2;
    d[6][i] = b[3] * f6 - b[2] * f2;
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[1][i] = b1[4] * f7 + b1[7] * f1;
    d[5][i] = b1[5] * f3 + b1[6] * f5;
    d[7][i] = b1[7] * f7 - b1[4] * f1;
    d[3][i] = b1[6] * f3 - b1[5] * f5;
  }
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      *(coefs + j + i * 8) = (short) floor(d[i][j] + 0.5);
    }
  }
  return;
}

#define divide_bits(d, n) ((n) < 0 ? (d) << (n) : (d) >> (n))

#if DWTDCT_TYPE == DWTDCT16X16_LEAN

void vp9_short_fdct32x32_c(short *input, short *out, int pitch) {
  // assume out is a 32x32 buffer
  short buffer[16 * 16];
  int i, j;
  const int short_pitch = pitch >> 1;
#if DWT_TYPE == 26
  dyadic_analyze_26(1, 32, 32, input, short_pitch, out, 32);
#elif DWT_TYPE == 97
  dyadic_analyze_97(1, 32, 32, input, short_pitch, out, 32);
#elif DWT_TYPE == 53
  dyadic_analyze_53(1, 32, 32, input, short_pitch, out, 32);
#endif
  // TODO(debargha): Implement more efficiently by adding output pitch
  // argument to the dct16x16 function
  vp9_short_fdct16x16_c_f(out, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 32, buffer + i * 16, sizeof(short) * 16);
  for (i = 0; i < 16; ++i) {
    for (j = 16; j < 32; ++j) {
      out[i * 32 + j] = divide_bits(out[i * 32 + j], DWT_PRECISION_BITS - 2);
    }
  }
  for (i = 16; i < 32; ++i) {
    for (j = 0; j < 32; ++j) {
      out[i * 32 + j] = divide_bits(out[i * 32 + j], DWT_PRECISION_BITS - 2);
    }
  }
}

#elif DWTDCT_TYPE == DWTDCT16X16

void vp9_short_fdct32x32_c(short *input, short *out, int pitch) {
  // assume out is a 32x32 buffer
  short buffer[16 * 16];
  int i, j;
  const int short_pitch = pitch >> 1;
#if DWT_TYPE == 26
  dyadic_analyze_26(1, 32, 32, input, short_pitch, out, 32);
#elif DWT_TYPE == 97
  dyadic_analyze_97(1, 32, 32, input, short_pitch, out, 32);
#elif DWT_TYPE == 53
  dyadic_analyze_53(1, 32, 32, input, short_pitch, out, 32);
#endif
  // TODO(debargha): Implement more efficiently by adding output pitch
  // argument to the dct16x16 function
  vp9_short_fdct16x16_c_f(out, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 32, buffer + i * 16, sizeof(short) * 16);
  vp9_short_fdct16x16_c_f(out + 16, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 32 + 16, buffer + i * 16, sizeof(short) * 16);

  vp9_short_fdct16x16_c_f(out + 32 * 16, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 32 + 32 * 16, buffer + i * 16, sizeof(short) * 16);

  vp9_short_fdct16x16_c_f(out + 33 * 16, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 32 + 33 * 16, buffer + i * 16, sizeof(short) * 16);
}

#elif DWTDCT_TYPE == DWTDCT8X8

void vp9_short_fdct32x32_c(short *input, short *out, int pitch) {
  // assume out is a 32x32 buffer
  short buffer[8 * 8];
  int i, j;
  const int short_pitch = pitch >> 1;
#if DWT_TYPE == 26
  dyadic_analyze_26(2, 32, 32, input, short_pitch, out, 32);
#elif DWT_TYPE == 97
  dyadic_analyze_97(2, 32, 32, input, short_pitch, out, 32);
#elif DWT_TYPE == 53
  dyadic_analyze_53(2, 32, 32, input, short_pitch, out, 32);
#endif
  // TODO(debargha): Implement more efficiently by adding output pitch
  // argument to the dct16x16 function
  vp9_short_fdct8x8_c_f(out, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 8; ++i)
    vpx_memcpy(out + i * 32, buffer + i * 8, sizeof(short) * 8);

  vp9_short_fdct8x8_c_f(out + 8, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 8; ++i)
    vpx_memcpy(out + i * 32 + 8, buffer + i * 8, sizeof(short) * 8);

  vp9_short_fdct8x8_c_f(out + 32 * 8, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 8; ++i)
    vpx_memcpy(out + i * 32 + 32 * 8, buffer + i * 8, sizeof(short) * 8);

  vp9_short_fdct8x8_c_f(out + 33 * 8, buffer, 64, 1 + DWT_PRECISION_BITS);
  for (i = 0; i < 8; ++i)
    vpx_memcpy(out + i * 32 + 33 * 8, buffer + i * 8, sizeof(short) * 8);

  for (i = 0; i < 16; ++i) {
    for (j = 16; j < 32; ++j) {
      out[i * 32 + j] = divide_bits(out[i * 32 + j], DWT_PRECISION_BITS - 2);
    }
  }
  for (i = 16; i < 32; ++i) {
    for (j = 0; j < 32; ++j) {
      out[i * 32 + j] = divide_bits(out[i * 32 + j], DWT_PRECISION_BITS - 2);
    }
  }
}

#endif

#if CONFIG_TX64X64
void vp9_short_fdct64x64_c(short *input, short *out, int pitch) {
  // assume out is a 64x64 buffer
  short buffer[16 * 16];
  int i, j;
  const int short_pitch = pitch >> 1;
#if DWT_TYPE == 26
  dyadic_analyze_26(2, 64, 64, input, short_pitch, out, 64);
#elif DWT_TYPE == 97
  dyadic_analyze_97(2, 64, 64, input, short_pitch, out, 64);
#elif DWT_TYPE == 53
  dyadic_analyze_53(2, 64, 64, input, short_pitch, out, 64);
#endif
  // TODO(debargha): Implement more efficiently by adding output pitch
  // argument to the dct16x16 function
  vp9_short_fdct16x16_c_f(out, buffer, 128, 2 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 64, buffer + i * 16, sizeof(short) * 16);

#if DWTDCT_TYPE == DWTDCT16X16_LEAN
  for (i = 0; i < 16; ++i) {
    for (j = 16; j < 48; ++j) {
      out[i * 64 + j] = divide_bits(out[i * 64 + j], DWT_PRECISION_BITS - 1);
    }
  }
  for (i = 16; i < 64; ++i) {
    for (j = 0; j < 64; ++j) {
      out[i * 64 + j] = divide_bits(out[i * 64 + j], DWT_PRECISION_BITS - 1);
    }
  }
#elif DWTDCT_TYPE == DWTDCT16X16
  vp9_short_fdct16x16_c_f(out + 16, buffer, 128, 2 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 64 + 16, buffer + i * 16, sizeof(short) * 16);

  vp9_short_fdct16x16_c_f(out + 64 * 16, buffer, 128, 2 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 64 + 64 * 16, buffer + i * 16, sizeof(short) * 16);

  vp9_short_fdct16x16_c_f(out + 65 * 16, buffer, 128, 2 + DWT_PRECISION_BITS);
  for (i = 0; i < 16; ++i)
    vpx_memcpy(out + i * 64 + 65 * 16, buffer + i * 16, sizeof(short) * 16);

  // There is no dct used on the highest bands for now.
  // Need to scale these coeffs by a factor of 2/2^DWT_PRECISION_BITS
  // TODO(debargha): experiment with turning these coeffs to 0
  for (i = 0; i < 32; ++i) {
    for (j = 32; j < 64; ++j) {
      out[i * 64 + j] = divide_bits(out[i * 64 + j], DWT_PRECISION_BITS - 1);
    }
  }
  for (i = 32; i < 64; ++i) {
    for (j = 0; j < 64; ++j) {
      out[i * 64 + j] = divide_bits(out[i * 64 + j], DWT_PRECISION_BITS - 1);
    }
  }
#endif  // DWTDCT_TYPE
}
#endif  // CONFIG_TX64X64
#endif  // CONFIG_DWTDCTHYBRID
