/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#ifndef MEDIASOURCECONSUMER_H
#define MEDIASOURCECONSUMER_H

#include <MediaSourceConsumer.h>
#include <node.h>
#include <string>

/*
 * Wrapper class of woogeen_base::MediaSourceConsumer
 */
class MediaSourceConsumer : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
  woogeen_base::MediaSourceConsumer* me;

 private:
  MediaSourceConsumer();
  ~MediaSourceConsumer();

  /*
   * Constructor.
   * Constructs a MediaSourceConsumer
   */
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  /*
   * Closes the MediaSourceConsumer.
   * The object cannot be used after this call
   */
  static v8::Handle<v8::Value> close(const v8::Arguments& args);
};

#endif
