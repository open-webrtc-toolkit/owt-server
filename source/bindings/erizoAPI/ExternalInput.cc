#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include <node.h>
#include "ExternalInput.h"


using namespace v8;

ExternalInput::ExternalInput() {};
ExternalInput::~ExternalInput() {};

void ExternalInput::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("ExternalInput"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"), FunctionTemplate::New(close)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("init"), FunctionTemplate::New(init)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setAudioReceiver"), FunctionTemplate::New(setAudioReceiver)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setVideoReceiver"), FunctionTemplate::New(setVideoReceiver)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("ExternalInput"), constructor);
}

Handle<Value> ExternalInput::New(const Arguments& args) {
  HandleScope scope;

  v8::String::Utf8Value param(args[0]->ToString());
  std::string url = std::string(*param);

  ExternalInput* obj = new ExternalInput();
  obj->me = new erizo::ExternalInput(url);
  obj->me->setStatusListener(obj);

  obj->Wrap(args.This());

  uv_async_init(uv_default_loop(), &obj->async_, &ExternalInput::statusCallback);

  return args.This();
}

Handle<Value> ExternalInput::close(const Arguments& args) {
  HandleScope scope;

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.This());
  erizo::ExternalInput *me = (erizo::ExternalInput*)obj->me;

  delete me;

  return scope.Close(Null());
}

Handle<Value> ExternalInput::init(const Arguments& args) {
  HandleScope scope;

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.This());
  erizo::ExternalInput *me = (erizo::ExternalInput*) obj->me;

  int r = me->init();
  obj->statusCallback_ = Persistent<Function>::New(Local<Function>::Cast(args[0]));

  return scope.Close(Integer::New(r));
}

Handle<Value> ExternalInput::setAudioReceiver(const Arguments& args) {
  HandleScope scope;

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.This());
  erizo::ExternalInput *me = (erizo::ExternalInput*)obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me->setAudioSink(mr);

  return scope.Close(Null());
}

Handle<Value> ExternalInput::setVideoReceiver(const Arguments& args) {
  HandleScope scope;

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.This());
  erizo::ExternalInput *me = (erizo::ExternalInput*)obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me->setVideoSink(mr);

  return scope.Close(Null());
}

void ExternalInput::notifyStatus(const std::string& message) {
  boost::mutex::scoped_lock lock(statusMutex);
  this->statusMsg = message;
  async_.data = this;
  uv_async_send (&async_);
}

void ExternalInput::statusCallback(uv_async_t *handle, int status){
  HandleScope scope;
  ExternalInput* obj = (ExternalInput*)handle->data;
  if (!obj)
    return;
  boost::mutex::scoped_lock lock(obj->statusMutex);
  Local<Value> args[] = {String::NewSymbol(obj->statusMsg.c_str())};
  obj->statusCallback_->Call(Context::GetCurrent()->Global(), 1, args);
}
