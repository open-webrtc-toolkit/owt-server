#ifndef WEBRTCCONNECTION_H_
#define WEBRTCCONNECTION_H_

#include <EventRegistry.h>
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
    CONN_INITIAL = 101,
    CONN_STARTED = 102,
    CONN_GATHERED = 103,
    CONN_READY = 104,
    CONN_FINISHED = 105,
    CONN_CANDIDATE = 201,
    CONN_SDP = 202,
    CONN_FAILED = 500
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
    WebRtcConnection(bool audioEnabled, bool videoEnabled, bool h264Enabled, const std::string &stunServer, int stunPort, int minPort, int maxPort, const std::string& certFile, const std::string& keyFile, const std::string& privatePasswd, uint32_t qos, bool trickleEnabled, const std::string& networkInterfaces, EventRegistry*);
    /**
     * Destructor.
     */
    virtual ~WebRtcConnection();
    /**
     * Inits the WebConnection by starting ICE Candidate Gathering.
     * @return True if the candidates are gathered.
     */
    bool init();

    void close();

    /**
     * Sets the SDP of the remote peer.
     * @param sdp The SDP.
     * @return true if the SDP was received correctly.
     */
    bool setRemoteSdp(const std::string &sdp);

    /**
     * Add new remote candidate (from remote peer).
     * @param sdp The candidate in SDP format.
     * @return true if the SDP was received correctly or successfully queued.
     */
    bool addRemoteCandidate(const std::string &mid, int mLineIndex, const std::string &sdp);
    /**
     * Obtains the local SDP.
     * @return The SDP as a string.
     */
    std::string getLocalSdp();
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

    int deliverAudioData(char* buf, int len);
    int deliverVideoData(char* buf, int len);
    int deliverFeedback(char* buf, int len);

    int preferredAudioPayloadType();
    int preferredVideoPayloadType();
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
     * Gets the current state of the Ice Connection
     * @return
     */
    WebRTCEvent getCurrentState();

    std::string getJSONStats();

    void onTransportData(char* buf, int len, Transport *transport);
    void updateState(TransportState state, Transport * transport);
    void queueData(int comp, const char* data, int len, Transport *transport, packetType type);
    void onCandidate(const CandidateInfo& cand, Transport *transport);

private:
    static const int STATS_INTERVAL = 5000;
    SdpInfo remoteSdp_;
    SdpInfo localSdp_;
    Stats thisStats_;
    WebRTCEvent globalState_;
    EventRegistry* asyncHandle_;

    int bundle_, sequenceNumberFIR_;
    boost::mutex writeMutex_, receiveMediaMutex_, updateStateMutex_;
    boost::thread send_Thread_;
    std::queue<dataPacket> sendQueue_;
    Transport *videoTransport_, *audioTransport_;
    boost::scoped_array<char> deliverMediaBuffer_;

    volatile bool sending_, closed_;
    void sendLoop();
    void writeSsrc(char* buf, int len, unsigned int ssrc);
    void processRtcpHeaders(char* buf, int len, unsigned int ssrc);
    std::string getJSONCandidate(const std::string& mid, const std::string& sdp);

    // changes the outgoing payload type for in the given data packet
    void changeDeliverPayloadType(dataPacket *dp, packetType type);
    // parses incoming payload type, replaces occurence in buf
    void parseIncomingPayloadType(char *buf, int len, packetType type);

    // Push pending remote candidates to transport layer.
    void drainPendingRemoteCandidates();

    bool audioEnabled_;
    bool videoEnabled_;
    bool nackEnabled_;
    bool trickleEnabled_;

    int stunPort_, minPort_, maxPort_;
    std::string stunServer_;
    std::string certFile_;
    std::string keyFile_;
    std::string privatePasswd_;

    boost::condition_variable cond_;

    // Indicates ICE restarting is in progress.
    // This value is true when we receive an offer from client side that
    // indicates an ICE restart. It will be set to false after sending out local
    // SDP.
    // TODO: Ack is not implemented. It should be set to false after client
    // successfully set MCU's SDP, then |pendingRemoteCandidates_| can be
    // drained. At this time, |pendingRemoteCandidates_| will never be drained.
    // However, it's not a problem for now, since we expect client start STUN
    // binding requests.
    // When ICE restart happens, we hope |pendingRemoteCandidates_| can be pushed
    // to nice connection after corresponding generation of SDP has been acked
    // by client side. Otherwise, 401 (Unauthorized) error may occur because ICE
    // restart changes credentials.
    bool iceRestarting_;
    // Remote candidates that haven't been set to nice connections.
    std::vector<CandidateInfo> pendingRemoteCandidates_;
    boost::mutex pendingRemoteCandidatesMutex_;
    // Remote ICE username fragment.
    std::string remoteUfrag_;
    int localSdpGeneration_;
    // PeerConnection only collects candidates on specific network interface.
    // If it is an empty string, collecting candidate on all network interfaces.
    std::string networkInterface_;
};

} /* namespace erizo */
#endif /* WEBRTCCONNECTION_H_ */
