#ifndef ERIZOAPI_WEBRTCCONNECTION_H_
#define ERIZOAPI_WEBRTCCONNECTION_H_

#include <nan.h>
#include <WebRtcConnection.h>
#include <MediaStream.h>
#include "MediaDefinitions.h"
// #include "OneToManyProcessor.h"

#include <queue>
#include <string>
#include <future>  // NOLINT

/*
 * Wrapper class of erizo::WebRtcConnection
 *
 * A WebRTC Connection. This class represents a WebRtcConnection that can be established with other peers via a SDP negotiation
 * it comprises all the necessary ICE and SRTP components.
 */
class WebRtcConnection : public MediaSink, public erizo::WebRtcConnectionEventListener {
 public:
    static NAN_MODULE_INIT(Init);

    std::shared_ptr<erizo::WebRtcConnection> me;
    int eventSt;
    std::queue<int> eventSts;
    std::queue<std::pair<std::string, std::string>> eventMsgs;

    boost::mutex mutex;

 private:
    WebRtcConnection();
    ~WebRtcConnection();

    Nan::Callback *eventCallback_;

    uv_async_t async_;
    uv_async_t asyncStats_;
    bool hasCallback_;
    /*
     * Constructor.
     * Constructs an empty WebRtcConnection without any configuration.
     */
    static NAN_METHOD(New);
    /*
     * Stop the webRTC connection.
     * The object cannot be used after this call.
     * Source and Sink of the connection can be destroyed after this call.
     */
    static NAN_METHOD(stop);
    /*
     * Closes the webRTC connection.
     * The object cannot be used after this call.
     */
    static NAN_METHOD(close);
    /*
     * Inits the WebRtcConnection and passes the callback to get Events.
     * Returns true if the candidates are gathered.
     */
    static NAN_METHOD(init);
    /*
     * Creates an SDP Offer
     * Param: No params.
     * Returns true if the process has started successfully.
     */
    static NAN_METHOD(createOffer);
    /*
     * Sets the SDP of the remote peer.
     * Param: the SDP.
     * Returns true if the SDP was received correctly.
     */
    static NAN_METHOD(setRemoteSdp);
    /**
     * Add new remote candidate (from remote peer).
     * @param sdp The candidate in SDP format.
     * @return true if the SDP was received correctly.
     */
    static NAN_METHOD(addRemoteCandidate);
    /**
     * Remove remote candidate (from remote peer).
     * @param sdp The candidate in SDP format.
     * @return true if the SDP was received correctly.
     */
    static NAN_METHOD(removeRemoteCandidate);

    static NAN_METHOD(addMediaStream);
    static NAN_METHOD(removeMediaStream);

    /*
     * SSRC get and set on SDP
     */
    static NAN_METHOD(setAudioSsrc);
    static NAN_METHOD(setVideoSsrcList);
    static NAN_METHOD(getAudioSsrcMap);
    static NAN_METHOD(getVideoSsrcMap);

    /*
     * Obtains the local SDP.
     * Returns the SDP as a string.
     */
    static NAN_METHOD(getLocalSdp);
    /*
     * Gets the current state of the Ice Connection
     * Returns the state.
     */
    static NAN_METHOD(getCurrentState);

    static Nan::Persistent<v8::Function> constructor;

    static NAUV_WORK_CB(eventsCallback);

    virtual void notifyEvent(erizo::WebRTCEvent event,
                             const std::string& message = "",
                             const std::string& stream_id = "");
};

#endif  // ERIZOAPI_WEBRTCCONNECTION_H_
