/*
 * ManyToManyTranscoder.cpp
 *
 *  Created on: Mar 7, 2014
 *      Author: qzhang8
 */

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include "node.h"
#include "ManyToManyTranscoder.h"
#include <VideoMixer.h>

using namespace v8;

ManyToManyTranscoder::ManyToManyTranscoder() {};
ManyToManyTranscoder::~ManyToManyTranscoder() {};

void ManyToManyTranscoder::Init(Handle<Object> target) {
	  // Prepare constructor template
	  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	  tpl->SetClassName(String::NewSymbol("ManyToManyTranscoder"));
	  tpl->InstanceTemplate()->SetInternalFieldCount(1);
	  // Prototype
	  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"), FunctionTemplate::New(close)->GetFunction());
	  tpl->PrototypeTemplate()->Set(String::NewSymbol("addPublisher"), FunctionTemplate::New(addPublisher)->GetFunction());
	  tpl->PrototypeTemplate()->Set(String::NewSymbol("removePublisher"), FunctionTemplate::New(removePublisher)->GetFunction());
	  tpl->PrototypeTemplate()->Set(String::NewSymbol("addSubscriber"), FunctionTemplate::New(addSubscriber)->GetFunction());
	  tpl->PrototypeTemplate()->Set(String::NewSymbol("removeSubscriber"), FunctionTemplate::New(removeSubscriber)->GetFunction());
	  tpl->PrototypeTemplate()->Set(String::NewSymbol("sendFIR"), FunctionTemplate::New(sendFIR)->GetFunction());

	  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	  target->Set(String::NewSymbol("ManyToManyTranscoder"), constructor);

}

Handle<Value> ManyToManyTranscoder::New(const Arguments& args) {

	  HandleScope scope;

	  ManyToManyTranscoder* obj = new ManyToManyTranscoder();
	  obj->me = new mcu::VideoMixer();

	  obj->Wrap(args.This());

	  return args.This();

}

Handle<Value> ManyToManyTranscoder::close(const Arguments& args) {
	HandleScope scope;

	ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
	mcu::VideoMixer *me = obj->me;
	delete me;

	return scope.Close(Null());

}

Handle<Value> ManyToManyTranscoder::addPublisher(
		const Arguments& args) {
	HandleScope scope;
	ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
	mcu::VideoMixer *me = (mcu::VideoMixer*)obj->me;

	WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
	erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param->me;

	erizo::MediaSource* ms = dynamic_cast<erizo::MediaSource*>(wr);
	me->addPublisher(ms);

	return scope.Close(Null());
}

Handle<Value> ManyToManyTranscoder::removePublisher(
		const Arguments& args) {
	HandleScope scope;
	ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
	mcu::VideoMixer *me = (mcu::VideoMixer*)obj->me;

	WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
	erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param->me;

	erizo::MediaSource* ms = dynamic_cast<erizo::MediaSource*>(wr);
	me->removePublisher(ms);

	return scope.Close(Null());
}

Handle<Value> ManyToManyTranscoder::addSubscriber(
		const Arguments& args) {
	HandleScope scope;
	ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
	mcu::VideoMixer *me = (mcu::VideoMixer*)obj->me;

	WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
	erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param->me;

	erizo::MediaSink* ms = dynamic_cast<erizo::MediaSink*>(wr);

	// get the param
	v8::String::Utf8Value param1(args[1]->ToString());

	// convert it to string
	std::string peerId = std::string(*param1);

	me->addSubscriber(ms, peerId);

	return scope.Close(Null());

}

Handle<Value> ManyToManyTranscoder::removeSubscriber(
		const Arguments& args) {
	HandleScope scope;
	ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
	mcu::VideoMixer *me = (mcu::VideoMixer*)obj->me;

	// get the param
	v8::String::Utf8Value param1(args[0]->ToString());

	// convert it to string
	std::string peerId = std::string(*param1);
	me->removeSubscriber(peerId);

	return scope.Close(Null());

}

Handle<Value> ManyToManyTranscoder::sendFIR(const Arguments& args) {
	HandleScope scope;
	return scope.Close(Null());

}
