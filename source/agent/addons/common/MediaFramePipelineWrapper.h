// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0


#ifndef MEDIAFRAMEPIPELINEWRAPPER_H
#define MEDIAFRAMEPIPELINEWRAPPER_H

#include <node.h>
#include <node_object_wrap.h>
#include <MediaFramePipeline.h>
#include <nan.h>


/*
 * Wrapper class of owt_base::FrameDestination
 */
class FrameDestination : public node::ObjectWrap{
public:

  owt_base::FrameDestination* dest;
};


/*
 * Wrapper class of owt_base::FrameSource
 */
class FrameSource : public node::ObjectWrap{
public:

  owt_base::FrameSource* src;
};

/*
 * Nan::ObjectWrap of owt_base::FrameSource and owt_base::FrameDestination, represents a node in the media or data pipeline.
 */
class NanFrameNode : public Nan::ObjectWrap {
public:
    virtual owt_base::FrameSource* FrameSource() = 0;
    virtual owt_base::FrameDestination* FrameDestination() = 0;
};


#endif