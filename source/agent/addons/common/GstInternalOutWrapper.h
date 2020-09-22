// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0


#ifndef GSTINTERNALOUTWRAPPER_H
#define GSTINTERNALOUTWRAPPER_H

#include <node.h>
#include <node_object_wrap.h>
#include <InternalOut.h>


/*
 * Wrapper class of owt_base::InternalOut
 */
class InternalOut : public node::ObjectWrap{
public:

  owt_base::InternalOut* out;
};

#endif

