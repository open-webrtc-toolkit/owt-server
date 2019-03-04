// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0


#ifndef MEDIAFRAMEPIPELINEWRAPPER_H
#define MEDIAFRAMEPIPELINEWRAPPER_H

#include <node.h>
#include <node_object_wrap.h>
#include <MediaFramePipeline.h>


/*
 * Wrapper class of woogeen_base::FrameDestination
 */
class FrameDestination : public node::ObjectWrap{
public:

  woogeen_base::FrameDestination* dest;
};


/*
 * Wrapper class of woogeen_base::FrameSource
 */
class FrameSource : public node::ObjectWrap{
public:

  woogeen_base::FrameSource* src;
};


#endif