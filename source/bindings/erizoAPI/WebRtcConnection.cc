#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "WebRtcConnection.h"


using namespace v8;

WebRtcConnection::WebRtcConnection() {};
WebRtcConnection::~WebRtcConnection(){
};

void WebRtcConnection::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("WebRtcConnection"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"), FunctionTemplate::New(close)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("init"), FunctionTemplate::New(init)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("isInitialized"), FunctionTemplate::New(isInitialized)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setRemoteSdp"), FunctionTemplate::New(setRemoteSdp)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getLocalSdp"), FunctionTemplate::New(getLocalSdp)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setAudioReceiver"), FunctionTemplate::New(setAudioReceiver)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setVideoReceiver"), FunctionTemplate::New(setVideoReceiver)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getCurrentState"), FunctionTemplate::New(getCurrentState)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getStats"), FunctionTemplate::New(getStats)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("WebRtcConnection"), constructor);
}


Handle<Value> WebRtcConnection::New(const Arguments& args) {
  HandleScope scope;
  if (args.Length()<13){
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return args.This();
  }
  //	webrtcconnection(bool audioEnabled, bool videoEnabled, const std::string &stunServer, int stunPort, int minPort, int maxPort);

  bool a = (args[0]->ToBoolean())->BooleanValue();
  bool v = (args[1]->ToBoolean())->BooleanValue();
  String::Utf8Value param2(args[2]->ToString());
  std::string stunServer = std::string(*param2);
  int stunPort = args[3]->IntegerValue();
  int minPort = args[4]->IntegerValue();
  int maxPort = args[5]->IntegerValue();
  String::Utf8Value param6(args[6]->ToString());
  std::string certFile = std::string(*param6);
  String::Utf8Value param7(args[7]->ToString());
  std::string keyFile = std::string(*param7);
  String::Utf8Value param8(args[8]->ToString());
  std::string privatePass = std::string(*param8);

  bool red = (args[9]->ToBoolean())->BooleanValue();
  bool fec = (args[10]->ToBoolean())->BooleanValue();
  bool nackSender = (args[11]->ToBoolean())->BooleanValue();
  bool nackRecv = (args[12]->ToBoolean())->BooleanValue();

  uint32_t qos = (red << erizo::QOS_SUPPORT_RED_SHIFT) |
                 (fec << erizo::QOS_SUPPORT_FEC_SHIFT) |
                 (nackSender << erizo::QOS_SUPPORT_NACK_SENDER_SHIFT) |
                 (nackRecv << erizo::QOS_SUPPORT_NACK_RECEIVER_SHIFT);

  WebRtcConnection* obj = new WebRtcConnection();
  obj->me = new erizo::WebRtcConnection(a, v, stunServer,stunPort,minPort,maxPort, certFile, keyFile, privatePass, qos);
  obj->me->setWebRtcConnectionEventListener(obj);
  obj->Wrap(args.This());
  uv_async_init(uv_default_loop(), &obj->async_, &WebRtcConnection::eventsCallback); 
  uv_async_init(uv_default_loop(), &obj->asyncStats_, &WebRtcConnection::statsCallback); 
  obj->statsMsg = "";
  return args.This();
}

Handle<Value> WebRtcConnection::close(const Arguments& args) {
  HandleScope scope;
  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;
  uv_close((uv_handle_t*)&obj->async_, NULL);
  uv_close((uv_handle_t*)&obj->asyncStats_, NULL);

  return scope.Close(Null());
}

Handle<Value> WebRtcConnection::init(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  bool r = me->init();
  obj->eventCallback_ = Persistent<Function>::New(Local<Function>::Cast(args[0]));

  return scope.Close(Boolean::New(r));
}

Handle<Value> WebRtcConnection::isInitialized(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  bool r = me->isInitialized();

  return scope.Close(Boolean::New(r));
}

Handle<Value> WebRtcConnection::setRemoteSdp(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string sdp = std::string(*param);

  bool r = me->setRemoteSdp(sdp);

  return scope.Close(Boolean::New(r));
}

Handle<Value> WebRtcConnection::getLocalSdp(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  std::string sdp = me->getLocalSdp();

  return scope.Close(String::NewSymbol(sdp.c_str()));
}


Handle<Value> WebRtcConnection::setAudioReceiver(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me-> setAudioSink(mr);

  return scope.Close(Null());
}

Handle<Value> WebRtcConnection::setVideoReceiver(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me-> setVideoSink(mr);

  return scope.Close(Null());
}

Handle<Value> WebRtcConnection::getCurrentState(const Arguments& args) {
  HandleScope scope;

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  erizo::WebRtcConnection *me = obj->me;

  int state = me->getCurrentState();

  return scope.Close(Number::New(state));
}

Handle<Value> WebRtcConnection::getStats(const v8::Arguments& args){
  HandleScope scope;
  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.This());
  obj->me->setWebRtcConnectionStatsListener(obj);
  obj->hasCallback_ = true;
  obj->statsCallback_ = Persistent<Function>::New(Local<Function>::Cast(args[0]));

  return scope.Close(Null());

}

void WebRtcConnection::notifyEvent(erizo::WebRTCEvent event, const std::string& message) {
  boost::mutex::scoped_lock lock(eventsMutex);
  this->eventSts.push(event);
  this->eventMsgs.push(message);
  async_.data = this;
  uv_async_send (&async_);
}

void WebRtcConnection::notifyStats(const std::string& message) {
  boost::mutex::scoped_lock lock(statsMutex);
  this->statsMsg=message;
  asyncStats_.data = this;
  uv_async_send (&asyncStats_);
}

void WebRtcConnection::eventsCallback(uv_async_t *handle, int status){
  HandleScope scope;
  WebRtcConnection* obj = (WebRtcConnection*)handle->data;
  if (!obj)
    return;
  boost::mutex::scoped_lock lock(obj->eventsMutex);
  while (!obj->eventSts.empty()) {
    Local<Value> args[] = {Integer::New(obj->eventSts.front()), String::NewSymbol(obj->eventMsgs.front().c_str())};
    obj->eventCallback_->Call(Context::GetCurrent()->Global(), 2, args);
    obj->eventMsgs.pop();
    obj->eventSts.pop();
  }
}

void WebRtcConnection::statsCallback(uv_async_t *handle, int status){

  HandleScope scope;
  WebRtcConnection* obj = (WebRtcConnection*)handle->data;

  Local<Value> args[] = {String::NewSymbol(obj->statsMsg.c_str())};
  if (obj->hasCallback_) 
    obj->statsCallback_->Call(Context::GetCurrent()->Global(), 1, args);
}
