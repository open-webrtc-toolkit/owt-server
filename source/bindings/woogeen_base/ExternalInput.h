#ifndef EXTERNALINPUT_H
#define EXTERNALINPUT_H

#include <ExternalInput.h>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

/*
 * Wrapper class of woogeen_base::ExternalInput
 */
class ExternalInput: public node::ObjectWrap, woogeen_base::ExternalInputStatusListener {
 public:
  static void Init(v8::Handle<v8::Object> exports);
  woogeen_base::ExternalInput* me;
  std::string statusMsg;
  boost::mutex statusMutex;

 private:
  ExternalInput();
  ~ExternalInput();
  static v8::Persistent<v8::Function> constructor;

  uv_async_t async_;
  v8::Persistent<v8::Function> statusCallback_;

  /*
   * Constructor.
   * Constructs a ExternalInput
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Closes the ExternalInput.
   * The object cannot be used after this call
   */
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Inits the ExternalInput 
   * Returns true ready
   */
  static void init(const v8::FunctionCallbackInfo<v8::Value>& args);  
  /*
   * Sets a MediaReceiver that is going to receive Audio Data
   * Param: the MediaReceiver to send audio to.
   */
  static void setAudioReceiver(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Sets a MediaReceiver that is going to receive Video Data
   * Param: the MediaReceiver
   */
  static void setVideoReceiver(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void statusCallback(uv_async_t *handle);

  virtual void notifyStatus(const std::string& message);
};

#endif
