// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
