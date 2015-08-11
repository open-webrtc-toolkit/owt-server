#ifndef EXTERNALINPUT_H
#define EXTERNALINPUT_H

#include "../erizoAPI/MediaDefinitions.h"
#include <node.h>
#include <ExternalInput.h>


/*
 * Wrapper class of woogeen_base::ExternalInput
 */
class ExternalInput: public node::ObjectWrap, woogeen_base::ExternalInputStatusListener {
 public:
  static void Init(v8::Handle<v8::Object> target);
  woogeen_base::ExternalInput* me;
  std::string statusMsg;
  boost::mutex statusMutex;

 private:
  ExternalInput();
  ~ExternalInput();

  uv_async_t async_;
  v8::Persistent<v8::Function> statusCallback_;

  /*
   * Constructor.
   * Constructs a ExternalInput
   */
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  /*
   * Closes the ExternalInput.
   * The object cannot be used after this call
   */
  static v8::Handle<v8::Value> close(const v8::Arguments& args);
  /*
   * Inits the ExternalInput 
   * Returns true ready
   */
  static v8::Handle<v8::Value> init(const v8::Arguments& args);  
  /*
   * Sets a MediaReceiver that is going to receive Audio Data
   * Param: the MediaReceiver to send audio to.
   */
  static v8::Handle<v8::Value> setAudioReceiver(const v8::Arguments& args);
  /*
   * Sets a MediaReceiver that is going to receive Video Data
   * Param: the MediaReceiver
   */
  static v8::Handle<v8::Value> setVideoReceiver(const v8::Arguments& args);

  static void statusCallback(uv_async_t *handle, int status);

  virtual void notifyStatus(const std::string& message);
};

#endif
