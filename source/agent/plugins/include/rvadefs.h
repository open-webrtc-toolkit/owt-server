/*
 * Copyright 2017 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef __RVADEFS_H__
#define __RVADEFS_H__

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

#define __INT64   long long
#define __UINT64  unsigned long long

#define RVA_CDECL

typedef unsigned char       rvaU8;
typedef char                rvaI8;
typedef short               rvaI16;
typedef unsigned short      rvaU16;
typedef unsigned int        rvaU32;
typedef int                 rvaI32;
typedef unsigned int        rvaUL32;
typedef int                 rvaL32;
typedef float               rvaF32;
typedef double              rvaF64;
typedef __UINT64            rvaU64;
typedef __INT64             rvaI64;
typedef void*               rvaHDL;
typedef char                rvaChar;

// Status code
typedef enum
{
    RVA_ERR_OK                = 0,
   
    RVA_ERR_UNKNOWN              = -1,
    RVA_ERR_NULL_PTR             = -2,
    RVA_ERR_UNSUPPORTED          = -3,
    RVA_ERR_NOT_INITIALIZED      = -4,
    RVA_ERR_MEMORY_ALLOC         = -5,
    RVA_ERR_ABORTED              = -6,
} rvaStatus;

#ifdef __cplusplus
} // extern "C"
#endif  /*__cplusplus */

#endif
