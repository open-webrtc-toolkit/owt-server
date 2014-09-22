/*
 * ManyToManyTranscoder.h
 *
 *  Created on: Mar 7, 2014
 *      Author: qzhang8
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
