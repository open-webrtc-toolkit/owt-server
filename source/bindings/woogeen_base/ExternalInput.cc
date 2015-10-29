#include "ExternalInput.h"
#include "../erizoAPI/MediaDefinitions.h"

using namespace v8;

Persistent<Function> ExternalInput::constructor;
ExternalInput::ExternalInput() {};
ExternalInput::~ExternalInput() {};

void ExternalInput::Init(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "ExternalInput"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "init", init);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setAudioReceiver", setAudioReceiver);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setVideoReceiver", setVideoReceiver);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "ExternalInput"), tpl->GetFunction());
}

void ExternalInput::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() == 0 || !args[0]->IsObject()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }

  Local<Object> optionObject = args[0]->ToObject();
  woogeen_base::ExternalInput::Options options{};

  Local<String> keyUrl = String::NewFromUtf8(isolate, "url");
  Local<String> keyTransport = String::NewFromUtf8(isolate, "transport");
  Local<String> keyBufferSize = String::NewFromUtf8(isolate, "buffer_size");
  Local<String> keyAudio = String::NewFromUtf8(isolate, "audio");
  Local<String> keyVideo = String::NewFromUtf8(isolate, "video");
  Local<String> keyH264 = String::NewFromUtf8(isolate, "h264");

  if (optionObject->Has(keyUrl))
    options.url = std::string(*String::Utf8Value(optionObject->Get(keyUrl)->ToString()));
  if (optionObject->Has(keyTransport) && !optionObject->Get(keyTransport)->IsUndefined())
    options.transport = std::string(*String::Utf8Value(optionObject->Get(keyTransport)->ToString()));
  if (optionObject->Has(keyBufferSize) && !optionObject->Get(keyBufferSize)->IsUndefined())
    options.bufferSize = optionObject->Get(keyBufferSize)->Uint32Value();
  if (optionObject->Has(keyAudio))
    options.enableAudio = (*optionObject->Get(keyAudio)->ToBoolean())->BooleanValue();
  if (optionObject->Has(keyVideo))
    options.enableVideo = (*optionObject->Get(keyVideo)->ToBoolean())->BooleanValue();
  if (optionObject->Has(keyH264))
    options.enableH264 = (*optionObject->Get(keyH264)->ToBoolean())->BooleanValue();

  ExternalInput* obj = new ExternalInput();
  obj->me = new woogeen_base::ExternalInput(options);
  obj->me->setStatusListener(obj);

  obj->Wrap(args.This());

  uv_async_init(uv_default_loop(), &obj->async_, &ExternalInput::statusCallback);

  args.GetReturnValue().Set(args.This());
}

void ExternalInput::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.Holder());
  woogeen_base::ExternalInput *me = (woogeen_base::ExternalInput*)obj->me;

  delete me;
}

void ExternalInput::init(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.Holder());
  woogeen_base::ExternalInput *me = (woogeen_base::ExternalInput*) obj->me;

  me->init();
  obj->statusCallback_.Reset(isolate, Local<Function>::Cast(args[0]));
}

void ExternalInput::setAudioReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.Holder());
  woogeen_base::ExternalInput *me = (woogeen_base::ExternalInput*)obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me->setAudioSink(mr);
}

void ExternalInput::setVideoReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExternalInput* obj = ObjectWrap::Unwrap<ExternalInput>(args.Holder());
  woogeen_base::ExternalInput *me = (woogeen_base::ExternalInput*)obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink *mr = param->msink;

  me->setVideoSink(mr);
}

void ExternalInput::notifyStatus(const std::string& message) {
  boost::mutex::scoped_lock lock(statusMutex);
  this->statusMsg = message;
  async_.data = this;
  uv_async_send(&async_);
}

void ExternalInput::statusCallback(uv_async_t *handle) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  ExternalInput* obj = (ExternalInput*)handle->data;
  if (!obj)
    return;
  boost::mutex::scoped_lock lock(obj->statusMutex);
  Local<Value> args[] = {String::NewFromUtf8(isolate, obj->statusMsg.c_str())};
  Local<Function>::New(isolate, obj->statusCallback_)->Call(isolate->GetCurrentContext()->Global(), 1, args);
}
