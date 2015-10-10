#ifndef WEBRTCCONNECTION_H
#define WEBRTCCONNECTION_H

#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <WebRtcConnection.h>

/*
 * Wrapper class of erizo::WebRtcConnection
 *
 * A WebRTC Connection. This class represents a WebRtcConnection that can be established with other peers via a SDP negotiation
 * it comprises all the necessary ICE and SRTP components.
 */
class WebRtcConnection : public node::ObjectWrap, erizo::WebRtcConnectionEventListener, erizo::WebRtcConnectionStatsListener  {
 public:
  static void Init(v8::Local<v8::Object> exports);

  erizo::WebRtcConnection *me;
  std::list<int> eventSts;
  std::list<std::string> eventMsgs;
  std::queue<std::string> statsMsgs;
  boost::mutex statsMutex, eventsMutex;

 private:
  WebRtcConnection();
  ~WebRtcConnection();
  static v8::Persistent<v8::Function> constructor;
  
  v8::Persistent<v8::Function> eventCallback_;
  v8::Persistent<v8::Function> statsCallback_;

  uv_async_t async_;
  uv_async_t asyncStats_;
  bool hasCallback_;
  /*
   * Constructor.
   * Constructs an empty WebRtcConnection without any configuration.
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Closes the webRTC connection.
   * The object cannot be used after this call.
   */
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Inits the WebRtcConnection and passes the callback to get Events.
   */
  static void init(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Sets the SDP of the remote peer.
   * Param: the SDP.
   * Returns true if the SDP was received correctly.
   */
  static void setRemoteSdp(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Starts the WebRtcConnection ICE Candidate Gathering.
   */
  static void start(const v8::FunctionCallbackInfo<v8::Value>& args);
  /**
     * Add new remote candidate (from remote peer).
     * @param sdp The candidate in SDP format.
     * @return true if the SDP was received correctly.
     */
  static void addRemoteCandidate(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Obtains the local SDP.
   * Returns the SDP as a string.
   */
  static void getLocalSdp(const v8::FunctionCallbackInfo<v8::Value>& args);
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
  /*
   * Gets the current state of the Ice Connection
   * Returns the state.
   */
  static void getCurrentState(const v8::FunctionCallbackInfo<v8::Value>& args);


  static void getStats(const v8::FunctionCallbackInfo<v8::Value>& args);
  
  static void eventsCallback(uv_async_t *handle);
  static void statsCallback(uv_async_t *handle);
 
	virtual void notifyEvent(erizo::WebRTCEvent event, const std::string& message="", bool prompt=false);
	virtual void notifyStats(const std::string& message);
};

#endif
