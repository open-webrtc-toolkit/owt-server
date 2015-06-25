#include "WebRtcConnection.h"
#include "MediaDefinitions.h"

using namespace v8;

Persistent<Function> WebRtcConnection::constructor;
WebRtcConnection::WebRtcConnection() {
};
WebRtcConnection::~WebRtcConnection(){
};

void WebRtcConnection::Init(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "WebRtcConnection"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "init", init);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setRemoteSdp", setRemoteSdp);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addRemoteCandidate", addRemoteCandidate);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getLocalSdp", getLocalSdp);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setAudioReceiver", setAudioReceiver);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setVideoReceiver", setVideoReceiver);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getCurrentState", getCurrentState);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getStats", getStats);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "WebRtcConnection"), tpl->GetFunction());
}

void WebRtcConnection::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  if (args.Length() < 15){
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }
  //	webrtcconnection(bool audioEnabled, bool videoEnabled, const std::string &stunServer, int stunPort, int minPort, int maxPort);

  bool a = (args[0]->ToBoolean())->BooleanValue();
  bool v = (args[1]->ToBoolean())->BooleanValue();
  bool h = (args[2]->ToBoolean())->BooleanValue();
  String::Utf8Value param3(args[3]->ToString());
  std::string stunServer = std::string(*param3);
  int stunPort = args[4]->IntegerValue();
  int minPort = args[5]->IntegerValue();
  int maxPort = args[6]->IntegerValue();
  String::Utf8Value param7(args[7]->ToString());
  std::string certFile = std::string(*param7);
  String::Utf8Value param8(args[8]->ToString());
  std::string keyFile = std::string(*param8);
  String::Utf8Value param9(args[9]->ToString());
  std::string privatePass = std::string(*param9);

  bool red = (args[10]->ToBoolean())->BooleanValue();
  bool fec = (args[11]->ToBoolean())->BooleanValue();
  bool nackSender = (args[12]->ToBoolean())->BooleanValue();
  bool nackRecv = (args[13]->ToBoolean())->BooleanValue();

  uint32_t qos = (red << erizo::QOS_SUPPORT_RED_SHIFT) |
                 (fec << erizo::QOS_SUPPORT_FEC_SHIFT) |
                 (nackSender << erizo::QOS_SUPPORT_NACK_SENDER_SHIFT) |
                 (nackRecv << erizo::QOS_SUPPORT_NACK_RECEIVER_SHIFT);

  bool t = (args[14]->ToBoolean())->BooleanValue();

  WebRtcConnection* obj = new WebRtcConnection();
  obj->me = new erizo::WebRtcConnection(a, v, h, stunServer,stunPort,minPort,maxPort, certFile, keyFile, privatePass, qos, t, obj);
  obj->Wrap(args.This());
  uv_async_init(uv_default_loop(), &obj->async_, &WebRtcConnection::eventsCallback); 
  uv_async_init(uv_default_loop(), &obj->asyncStats_, &WebRtcConnection::statsCallback); 
  args.GetReturnValue().Set(args.This());
}

void WebRtcConnection::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  obj->me = NULL;
  obj->hasCallback_ = false;
  
  if(!uv_is_closing((uv_handle_t*)&obj->async_)) {
    uv_close((uv_handle_t*)&obj->async_, NULL);
  }
  if(!uv_is_closing((uv_handle_t*)&obj->asyncStats_)) {
    uv_close((uv_handle_t*)&obj->asyncStats_, NULL);
  }

}

void WebRtcConnection::init(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  bool r = me->init();
  obj->eventCallback_.Reset(isolate, Local<Function>::Cast(args[0]));

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void WebRtcConnection::setRemoteSdp(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string sdp = std::string(*param);

  bool r = me->setRemoteSdp(sdp);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void WebRtcConnection::addRemoteCandidate(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string mid = std::string(*param);

  int sdpMLine = args[1]->IntegerValue();
  
  String::Utf8Value param2(args[2]->ToString());
  std::string sdp = std::string(*param2);

  bool r = me->addRemoteCandidate(mid, sdpMLine, sdp);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void WebRtcConnection::getLocalSdp(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  std::string sdp = me->getLocalSdp();

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, sdp.c_str()));
}

void WebRtcConnection::setAudioReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me->setAudioSink(mr);
}

void WebRtcConnection::setVideoReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me->setVideoSink(mr);
}

void WebRtcConnection::getCurrentState(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  erizo::WebRtcConnection *me = obj->me;

  int state = me->getCurrentState();

  args.GetReturnValue().Set(Number::New(isolate, state));
}

void WebRtcConnection::getStats(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  WebRtcConnection* obj = ObjectWrap::Unwrap<WebRtcConnection>(args.Holder());
  if (obj->me == NULL){ //Requesting stats when WebrtcConnection not available
    return;
  }
  if (args.Length()==0){
    std::string lastStats = obj->me->getJSONStats();
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, lastStats.c_str()));
  }else{
    obj->me->setWebRtcConnectionStatsListener(obj);
    obj->hasCallback_ = true;
    obj->statsCallback_.Reset(isolate, Local<Function>::Cast(args[0]));
  }
}

void WebRtcConnection::notifyEvent(erizo::WebRTCEvent event, const std::string& message, bool prompt) {
  boost::mutex::scoped_lock lock(eventsMutex);
  if (prompt) {
    this->eventSts.push_front(event);
    this->eventMsgs.push_front(message);
  } else {
    this->eventSts.push_back(event);
    this->eventMsgs.push_back(message);
  }
  async_.data = this;
  uv_async_send(&async_);
}

void WebRtcConnection::notifyStats(const std::string& message) {
  if (!this->hasCallback_){
    return;
  }
  boost::mutex::scoped_lock lock(statsMutex);
  this->statsMsgs.push(message);
  asyncStats_.data = this;
  uv_async_send(&asyncStats_);
}

void WebRtcConnection::eventsCallback(uv_async_t *handle){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  WebRtcConnection* obj = (WebRtcConnection*)handle->data;
  if (!obj || obj->me == NULL)
    return;
  boost::mutex::scoped_lock lock(obj->eventsMutex);
  while (!obj->eventSts.empty()) {
    Local<Value> args[] = {Integer::New(isolate, obj->eventSts.front()), String::NewFromUtf8(isolate, obj->eventMsgs.front().c_str())};
    Local<Function>::New(isolate, obj->eventCallback_)->Call(isolate->GetCurrentContext()->Global(), 2, args);
    obj->eventMsgs.pop_front();
    obj->eventSts.pop_front();
  }
}

void WebRtcConnection::statsCallback(uv_async_t *handle){

  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  WebRtcConnection* obj = (WebRtcConnection*)handle->data;
  if (!obj || obj->me == NULL)
    return;
  boost::mutex::scoped_lock lock(obj->statsMutex);
  if (obj->hasCallback_) {
    while(!obj->statsMsgs.empty()){
      Local<Value> args[] = {String::NewFromUtf8(isolate, obj->statsMsgs.front().c_str())};
      Local<Function>::New(isolate, obj->statsCallback_)->Call(isolate->GetCurrentContext()->Global(), 1, args);
      obj->statsMsgs.pop();
    }
  }
}
