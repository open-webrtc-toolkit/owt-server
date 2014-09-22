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

#ifndef MANYTOMANYTRANSCODER_H_
#define MANYTOMANYTRANSCODER_H_

#include <node.h>
#include "../erizoAPI/MediaDefinitions.h"
#include "../erizoAPI/WebRtcConnection.h"


namespace mcu {
  class VideoMixer;
}

class ManyToManyTranscoder: public node::ObjectWrap {

public:
 static void Init(v8::Handle<v8::Object> target);
 mcu::VideoMixer* me;


private:
 ManyToManyTranscoder() ;
 ~ManyToManyTranscoder();
 /*
  * Constructor.
  * Constructs a OneToManyProcessor
  */
 static v8::Handle<v8::Value> New(const v8::Arguments& args);
 /*
  * Closes the OneToManyProcessor.
  * The object cannot be used after this call
  */
 static v8::Handle<v8::Value> close(const v8::Arguments& args);
 /*
  * Add a new Publisher
  * Param: the WebRtcConnection of the Publisher
  */
 static v8::Handle<v8::Value> addPublisher(const v8::Arguments& args);
 /*
  * Remove a Publisher
  * Param: the WebRtcConnection of the Publisher
  */
 static v8::Handle<v8::Value> removePublisher(const v8::Arguments& args);

 /*
  * Sets the subscriber
  * Param1: the WebRtcConnection of the subscriber
  * Param2: an unique Id for the subscriber
  */
 static v8::Handle<v8::Value> addSubscriber(const v8::Arguments& args);
 /*
  * Removes a subscriber given its peer id
  * Param: the peerId
  */
 static v8::Handle<v8::Value> removeSubscriber(const v8::Arguments& args);
 /*
  * Ask the publisher to send a FIR packet
  */
 static v8::Handle<v8::Value> sendFIR(const v8::Arguments& args);

};


#endif /* MANYTOMANYTRANSCODER_H_ */
