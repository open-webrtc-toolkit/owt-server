#ifndef WEBRTCCONNECTION_H_
#define WEBRTCCONNECTION_H_

#include <string>
#include <queue>
#include <boost/scoped_array.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#include <logger.h>

#include "SdpInfo.h"
#include "MediaDefinitions.h"
#include "Transport.h"
#include "Stats.h"

namespace erizo {

class Transport;
class TransportListener;

static const uint8_t QOS_SUPPORT_RED_SHIFT = 0;
static const uint8_t QOS_SUPPORT_FEC_SHIFT = 1;
static const uint8_t QOS_SUPPORT_NACK_SENDER_SHIFT = 2;
static const uint8_t QOS_SUPPORT_NACK_RECEIVER_SHIFT = 3;

static const uint32_t QOS_SUPPORT_RED_MASK = 1 << QOS_SUPPORT_RED_SHIFT;
static const uint32_t QOS_SUPPORT_FEC_MASK = 1 << QOS_SUPPORT_FEC_SHIFT;
static const uint32_t QOS_SUPPORT_NACK_SENDER_MASK = 1 << QOS_SUPPORT_NACK_SENDER_SHIFT;
static const uint32_t QOS_SUPPORT_NACK_RECEIVER_MASK = 1 << QOS_SUPPORT_NACK_RECEIVER_SHIFT;

/**
 * WebRTC Events
 */
enum WebRTCEvent {
  CONN_INITIAL = 101, CONN_STARTED = 102, CONN_READY = 103, CONN_FINISHED = 104, 
  CONN_FAILED = 500
};

class WebRtcConnectionEventListener {
public:
	virtual ~WebRtcConnectionEventListener() {
	}
	;
	virtual void notifyEvent(WebRTCEvent newEvent, const std::string& message="")=0;

};

class WebRtcConnectionStatsListener {
public:
	virtual ~WebRtcConnectionStatsListener() {
	}
	;
	virtual void notifyStats(const std::string& message)=0;
};
/**
 * A WebRTC Connection. This class represents a WebRTC Connection that can be established with other peers via a SDP negotiation
 * it comprises all the necessary Transport components.
 */
class WebRtcConnection: public MediaSink, public MediaSource, public FeedbackSink, public FeedbackSource, public TransportListener {
	DECLARE_LOGGER();
public:
	/**
	 * Constructor.
	 * Constructs an empty WebRTCConnection without any configuration.
	 */
	DLL_PUBLIC WebRtcConnection(bool audioEnabled, bool videoEnabled, const std::string &stunServer, int stunPort, int minPort, int maxPort, const std::string& certFile, const std::string& keyFile, const std::string& privatePasswd, uint32_t qos);
	/**
	 * Destructor.
	 */
	virtual ~WebRtcConnection();

	/**
	 * Inits the WebConnection by starting ICE Candidate Gathering.
	 * @return True if the candidates are gathered.
	 */
	DLL_PUBLIC bool init();

	void close();

	/**
	 * Sets the SDP of the remote peer.
	 * @param sdp The SDP.
	 * @return true if the SDP was received correctly.
	 */
	DLL_PUBLIC bool setRemoteSdp(const std::string &sdp);
	/**
	 * Obtains the local SDP.
	 * @return The SDP as a string.
	 */
	DLL_PUBLIC std::string getLocalSdp();

	/**
	 * Try to use the specified audio codec exclusively.
	 * This should be invoked before SDP negotiation.
	 * Return the RTP payload type of the codec.
	 */
	int setAudioCodec(const std::string& codecName, unsigned int clockRate);
	/**
	 * Try to use the specified video codec exclusively.
	 * This should be invoked before SDP negotiation.
	 * Return the RTP payload type of the codec.
	 */
	int setVideoCodec(const std::string& codecName, unsigned int clockRate);

	void setInitialized(bool);

	DLL_PUBLIC bool isInitialized();

	int deliverAudioData(char* buf, int len, MediaSource*);
	int deliverVideoData(char* buf, int len, MediaSource*);

	int deliverFeedback(char* buf, int len);

	bool acceptEncapsulatedRTPData();
	bool acceptFEC();
	bool acceptResentData();

	bool acceptNACK();

	/**
	 * Sends a FIR Packet (RFC 5104) asking for a keyframe
	 * @return the size of the data sent
	 */
	int sendFirPacket();
  
  /**
   * Sets the Event Listener for this WebRtcConnection
   */

	inline void setWebRtcConnectionEventListener(
			WebRtcConnectionEventListener* listener){
    this->connEventListener_ = listener;
  }
	
  /**
   * Sets the Stats Listener for this WebRtcConnection
   */
  inline void setWebRtcConnectionStatsListener(
			WebRtcConnectionStatsListener* listener){
    this->statsListener_ = listener;
    this->thisStats_.setPeriodicStats(STATS_INTERVAL, listener);
  }
	/**
	 * Gets the current state of the Ice Connection
	 * @return
	 */
	DLL_PUBLIC WebRTCEvent getCurrentState();

	void onTransportData(char* buf, int len, Transport *transport);

	void updateState(TransportState state, Transport * transport);

	void queueData(int comp, const char* data, int len, Transport *transport);

private:
  static const int STATS_INTERVAL = 5000;
	SdpInfo remoteSdp_;
	SdpInfo localSdp_;

  Stats thisStats_;

	WebRTCEvent globalState_;
	bool initialized_;

	int bundle_, sequenceNumberFIR_;
	boost::mutex writeMutex_, receiveMediaMutex_, updateStateMutex_;
	boost::thread send_Thread_;
	std::queue<dataPacket> sendQueue_;
	WebRtcConnectionEventListener* connEventListener_;
  WebRtcConnectionStatsListener* statsListener_;
	Transport *videoTransport_, *audioTransport_;
	boost::scoped_array<char> deliverMediaBuffer_;

	bool sending_, closed_;
	void sendLoop();
	void writeSsrc(char* buf, int len, unsigned int ssrc);
	void processRtcpHeaders(char* buf, int len, unsigned int ssrc);

	bool audioEnabled_;
	bool videoEnabled_;
	bool nackEnabled_;

	int stunPort_, minPort_, maxPort_;
	std::string stunServer_;
	std::string certFile_;
	std::string keyFile_;
	std::string privatePasswd_;

	boost::condition_variable cond_;
	int id_;

};

} /* namespace erizo */
#endif /* WEBRTCCONNECTION_H_ */
