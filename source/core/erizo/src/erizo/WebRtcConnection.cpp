/*
 * WebRTCConnection.cpp
 */

#include <cstdio>

#include "WebRtcConnection.h"
#include "DtlsTransport.h"
#include "SdpInfo.h"
#include <rtputils.h>

namespace erizo {
  DEFINE_LOGGER(WebRtcConnection, "WebRtcConnection");

  WebRtcConnection::WebRtcConnection(bool audioEnabled, bool videoEnabled, bool h264Enabled, const std::string &stunServer, int stunPort, int minPort,
      int maxPort, const std::string& certFile, const std::string& keyFile, const std::string& privatePasswd, uint32_t qos, WebRtcConnectionEventListener* listener)
  : connEventListener_(listener) {

    ELOG_WARN("WebRtcConnection constructor stunserver %s stunPort %d minPort %d maxPort %d\n", stunServer.c_str(), stunPort, minPort, maxPort);
    sequenceNumberFIR_ = 0;
    bundle_ = false;
    this->setVideoSinkSSRC(55543);
    this->setAudioSinkSSRC(44444);
    videoSink_ = NULL;
    audioSink_ = NULL;
    fbSink_ = NULL;
    sourcefbSink_ = this;
    sinkfbSource_ = this;
    globalState_ = CONN_INITIAL;
    videoTransport_ = NULL;
    audioTransport_ = NULL;

    audioEnabled_ = audioEnabled;
    videoEnabled_ = videoEnabled;
    if (!h264Enabled) {
      localSdp_.removePayloadSupport(H264_90000_PT);
      remoteSdp_.removePayloadSupport(H264_90000_PT);
    }

    stunServer_ = stunServer;
    stunPort_ = stunPort;
    minPort_ = minPort;
    maxPort_ = maxPort;
    certFile_ = certFile;
    keyFile_ = keyFile;
    privatePasswd_ = privatePasswd;

    nackEnabled_ = qos & QOS_SUPPORT_NACK_RECEIVER_MASK;
    localSdp_.setNACKSupport(qos & QOS_SUPPORT_NACK_SENDER_MASK);

    localSdp_.setREDSupport(qos & QOS_SUPPORT_RED_MASK);
    localSdp_.setFECSupport(qos & QOS_SUPPORT_FEC_MASK);
    remoteSdp_.setREDSupport(qos & QOS_SUPPORT_RED_MASK);
    remoteSdp_.setFECSupport(qos & QOS_SUPPORT_FEC_MASK);

    sending_ = true;
    send_Thread_ = boost::thread(&WebRtcConnection::sendLoop, this);
  }

  WebRtcConnection::~WebRtcConnection() {
    ELOG_INFO("WebRtcConnection Destructor");
    sending_ = false;
    cond_.notify_one();
    send_Thread_.join();
    globalState_ = CONN_FINISHED;
    if (connEventListener_ != NULL){
      // Prompt this event notification because we want the listener
      // to get the information that we're being destroyed ASAP so that
      // it won't ask us to do something when it receives some other events (from other working threads).
      connEventListener_->notifyEvent(globalState_, "", true);
      connEventListener_ = NULL;
    }
    globalState_ = CONN_FINISHED;
    delete videoTransport_;
    videoTransport_=NULL;
    delete audioTransport_;
    audioTransport_= NULL;
    videoSink_ = NULL;
    audioSink_ = NULL;
    fbSink_ = NULL;
  }

  bool WebRtcConnection::init() {
    return true;
  }

  int WebRtcConnection::setAudioCodec(const std::string& codecName, unsigned int clockRate) {
    int payloadType = remoteSdp_.forceCodecSupportByName(codecName, clockRate, AUDIO_TYPE);
    if (payloadType == INVALID_PT) {
      ELOG_WARN("Fail to set audio codec %s, clock rate %u", codecName.c_str(), clockRate);
    }
    return payloadType;
  }

  int WebRtcConnection::setVideoCodec(const std::string& codecName, unsigned int clockRate) {
    int payloadType = remoteSdp_.forceCodecSupportByName(codecName, clockRate, VIDEO_TYPE);
    if (payloadType == INVALID_PT) {
      ELOG_WARN("Fail to set video codec %s, clock rate %u", codecName.c_str(), clockRate);
    }
    return payloadType;
  }

  bool WebRtcConnection::setRemoteSdp(const std::string& sdp) {
    ELOG_DEBUG("Set Remote SDP %s", sdp.c_str());
    remoteSdp_.initWithSdp(sdp, "");

    CryptoInfo cryptLocal_video;
    CryptoInfo cryptLocal_audio;
    CryptoInfo cryptRemote_video;
    CryptoInfo cryptRemote_audio;

    bool videoTransportNeeded = videoEnabled_ && remoteSdp_.videoDirection >= RECVONLY;
    bool audioTransportNeeded = audioEnabled_ && remoteSdp_.audioDirection >= RECVONLY && (!remoteSdp_.isBundle || !videoTransportNeeded);
    bundle_ = audioEnabled_ && remoteSdp_.audioDirection >= RECVONLY && remoteSdp_.isBundle && videoTransportNeeded;

    localSdp_.getPayloadInfos() = remoteSdp_.getPayloadInfos();
    localSdp_.isBundle = bundle_;
    localSdp_.isRtcpMux = remoteSdp_.isRtcpMux;
    localSdp_.setOfferSdp(remoteSdp_);

    ELOG_DEBUG("Video %d videossrc %u Audio %d audio ssrc %u Bundle %d", videoEnabled_, remoteSdp_.videoSsrc, audioEnabled_, remoteSdp_.audioSsrc,  bundle_);

    this->setVideoSourceSSRC(remoteSdp_.videoSsrc);
    this->thisStats_.setVideoSourceSSRC(this->getVideoSourceSSRC());
    this->setAudioSourceSSRC(remoteSdp_.audioSsrc);
    this->thisStats_.setAudioSourceSSRC(this->getAudioSourceSSRC());

    if (!remoteSdp_.supportPayloadType(RED_90000_PT))
      deliverMediaBuffer_.reset(new char[3000]);

    if (remoteSdp_.profile == SAVPF) {
      if (remoteSdp_.isFingerprint) {
        // DTLS-SRTP
        if (videoTransportNeeded) {
          std::string username, password;
          remoteSdp_.getCredentials(username, password, VIDEO_TYPE);
          videoTransport_ = new DtlsTransport(VIDEO_TYPE, "video", bundle_, remoteSdp_.isRtcpMux, this, stunServer_, stunPort_, minPort_, maxPort_, certFile_, keyFile_, privatePasswd_, username, password);
        }
        if (audioTransportNeeded) {
          std::string username, password;
          remoteSdp_.getCredentials(username, password, AUDIO_TYPE);
          audioTransport_ = new DtlsTransport(AUDIO_TYPE, "audio", false, remoteSdp_.isRtcpMux, this, stunServer_, stunPort_, minPort_, maxPort_, certFile_, keyFile_, privatePasswd_, username, password);
        }
      }

      // In case both audio and video transports are needed, we need to start
      // the transports after they are both created because the WebRtcConnection
      // state update needs information from both.
      if (videoTransport_ != NULL)
        videoTransport_->start();
      if (audioTransport_ != NULL)
        audioTransport_->start();
    }

    std::string object = this->getLocalSdp();
    if (connEventListener_)
      connEventListener_->notifyEvent(CONN_SDP, object);

    if (!remoteSdp_.getCandidateInfos().empty()){
      ELOG_DEBUG("There are candidate in the SDP: Setting Remote Candidates!!!!");
      if (videoTransport_ != NULL) {
        videoTransport_->setRemoteCandidates(remoteSdp_.getCandidateInfos(), bundle_);
      }
      if (audioTransport_ != NULL) {
        audioTransport_->setRemoteCandidates(remoteSdp_.getCandidateInfos(), bundle_);
      }
    }

    return true;
  }

  bool WebRtcConnection::addRemoteCandidate(const std::string &mid, int mLineIndex, const std::string &sdp) {
    // TODO Check type of transport.
    ELOG_DEBUG("Adding remote Candidate %s, mid %s, sdpMLine %d",sdp.c_str(), mid.c_str(), mLineIndex);
    MediaType theType;
    std::string theMid;
    if ((!mid.compare("video"))||(mLineIndex ==remoteSdp_.videoSdpMLine)){
      theType = VIDEO_TYPE;
      theMid = "video";
    }else{
      theType = AUDIO_TYPE;
      theMid = "audio";
    }
    SdpInfo tempSdp;
    std::string username, password;
    remoteSdp_.getCredentials(username, password, theType);
    tempSdp.setCredentials(username, password, OTHER);
    bool res = false;
    if(tempSdp.initWithSdp(sdp, theMid)){
      if (theType == VIDEO_TYPE||bundle_){
        ELOG_DEBUG("Setting VIDEO CANDIDATE" );
        res = videoTransport_->setRemoteCandidates(tempSdp.getCandidateInfos(), bundle_);
      } else if (theType==AUDIO_TYPE){
        ELOG_DEBUG("Setting AUDIO CANDIDATE");
        res = audioTransport_->setRemoteCandidates(tempSdp.getCandidateInfos(), bundle_);
      }else{
        ELOG_ERROR("Cannot add remote candidate with no Media (video or audio)");
      }
    }
    return res;
  }

  std::string WebRtcConnection::getLocalSdp() {
    ELOG_DEBUG("Setting SSRC to localSdp %u", this->getVideoSinkSSRC());
    localSdp_.videoSsrc = this->getVideoSinkSSRC();
    localSdp_.audioSsrc = this->getAudioSinkSSRC();

    ELOG_DEBUG("Getting SDP");
    if (videoTransport_ != NULL) {
      videoTransport_->processLocalSdp(&localSdp_);
    }
    ELOG_DEBUG("Video SDP done.");
    if (!bundle_ && audioTransport_ != NULL) {
      audioTransport_->processLocalSdp(&localSdp_);
    }
    ELOG_DEBUG("Audio SDP done.");

    return localSdp_.getSdp();
  }

  int WebRtcConnection::preferredAudioPayloadType() {
    return remoteSdp_.preferredPayloadType(AUDIO_TYPE);
  }

  int WebRtcConnection::preferredVideoPayloadType() {
    return remoteSdp_.preferredPayloadType(VIDEO_TYPE);
  }

  bool WebRtcConnection::acceptEncapsulatedRTPData() {
    return remoteSdp_.supportPayloadType(RED_90000_PT);
  }

  bool WebRtcConnection::acceptFEC() {
    return remoteSdp_.supportPayloadType(ULP_90000_PT);
  }

  bool WebRtcConnection::acceptResentData() {
    return localSdp_.supportNACK();
  }

  bool WebRtcConnection::acceptNACK() {
    return nackEnabled_ && remoteSdp_.supportNACK();
  }

  std::string WebRtcConnection::getJSONCandidate(const std::string& mid, const std::string& sdp) {
    std::map <std::string, std::string> object;
    object["sdpMid"] = mid;
    object["candidate"] = sdp;
    char lineIndex[1];
    sprintf(lineIndex,"%d",(mid.compare("video")?localSdp_.audioSdpMLine:localSdp_.videoSdpMLine));
    object["sdpMLineIndex"] = std::string(lineIndex);

    std::ostringstream theString;
    theString << "{";
    for (std::map<std::string, std::string>::iterator it=object.begin(); it!=object.end(); ++it){
      theString << "\"" << it->first << "\":\"" << it->second << "\"";
      if (++it != object.end()){
        theString << ",";
      }
      --it;
    }
    theString << "}";
    return theString.str();
  }

  void WebRtcConnection::onCandidate(const std::string& sdp, Transport *transport) {
    if (connEventListener_ != NULL) {
      if (!bundle_) {
        std::string object = this->getJSONCandidate(transport->transport_name, sdp);
        connEventListener_->notifyEvent(CONN_CANDIDATE, object);
      } else {
        if (remoteSdp_.hasAudio){
          std::string object = this->getJSONCandidate("audio", sdp);
          connEventListener_->notifyEvent(CONN_CANDIDATE, object);
        }
        if (remoteSdp_.hasVideo){
          std::string object2 = this->getJSONCandidate("video", sdp);
          connEventListener_->notifyEvent(CONN_CANDIDATE, object2);
        }
      }
      
    }
  }

  int WebRtcConnection::deliverAudioData(char* buf, int len) {
    writeSsrc(buf, len, this->getAudioSinkSSRC());
    if (bundle_){
      if (videoTransport_ != NULL) {
        if (audioEnabled_ == true) {
          this->queueData(0, buf, len, videoTransport_, AUDIO_PACKET);
        }
      }
    } else if (audioTransport_ != NULL) {
      if (audioEnabled_ == true) {
        this->queueData(0, buf, len, audioTransport_, AUDIO_PACKET);
      }
    }
    return len;
  }

  int WebRtcConnection::deliverVideoData(char* buf, int len) {
    RTPHeader* head = (RTPHeader*) buf;
    writeSsrc(buf, len, this->getVideoSinkSSRC());
    if (videoTransport_ != NULL) {
      if (videoEnabled_ == true) {

        if (head->getPayloadType() == RED_90000_PT) {
          int totalLength = head->getHeaderLength();
          int rtpHeaderLength = totalLength;
          redheader *redhead = (redheader*) (buf + totalLength);

          //redhead->payloadtype = remoteSdp_.inOutPTMap[redhead->payloadtype];
          if (!remoteSdp_.supportPayloadType(head->getPayloadType())) {
            assert(deliverMediaBuffer_ != NULL);
            char* sendBuffer = deliverMediaBuffer_.get();

            while (redhead->follow) {
              totalLength += redhead->getLength() + 4; // RED header
              redhead = (redheader*) (buf + totalLength);
            }
            // Parse RED packet to VP8 packet.
            // Copy RTP header
            memcpy(sendBuffer, buf, rtpHeaderLength);
            // Copy payload data
            memcpy(sendBuffer + totalLength, buf + totalLength + 1, len - totalLength - 1);
            // Copy payload type
            RTPHeader* mediahead = (RTPHeader*) sendBuffer;
            mediahead->setPayloadType(redhead->payloadtype);
            buf = sendBuffer;
            len = len - 1 - totalLength + rtpHeaderLength;
          }
        }
        this->queueData(0, buf, len, videoTransport_, VIDEO_PACKET);
      }
    }
    return len;
  }

  int WebRtcConnection::deliverFeedback(char* buf, int len){
    // Check where to send the feedback
    RTCPFeedbackHeader* chead = reinterpret_cast<RTCPFeedbackHeader*>(buf);
    uint32_t sourceSsrc = chead->getSourceSSRC();
    if (sourceSsrc == 0)
        return 0;

    ELOG_TRACE("received Feedback type %u, ssrc %u, sourceSsrc %u",
      chead->getRTCPHeader().getPacketType(), chead->getRTCPHeader().getSSRC(), sourceSsrc);
    if (sourceSsrc == getAudioSinkSSRC() || sourceSsrc == getAudioSourceSSRC()) {
      writeSsrc(buf, len, this->getAudioSinkSSRC());
      if (audioTransport_ != NULL)
        this->queueData(0, buf, len, audioTransport_, OTHER_PACKET);
      else {
        assert(bundle_ && videoTransport_ != NULL);
        this->queueData(0, buf, len, videoTransport_, OTHER_PACKET);
      }
    } else if (sourceSsrc == getVideoSinkSSRC() || sourceSsrc == getVideoSourceSSRC()) {
      writeSsrc(buf, len, this->getVideoSinkSSRC());
      assert(videoTransport_ != NULL);
      this->queueData(0, buf, len, videoTransport_, OTHER_PACKET);
    } else {
      ELOG_WARN("Bad source SSRC in RTCP feedback packet: %u", sourceSsrc);
    }
    return len;
  }

  void WebRtcConnection::writeSsrc(char* buf, int len, unsigned int ssrc) {
    RTPHeader* head = (RTPHeader*) buf;
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*> (buf);
    uint8_t packetType = chead->getPacketType();
    //if it is RTCP we check it it is a compound packet
    if (packetType == RTCP_Sender_PT || packetType == RTCP_Receiver_PT || packetType == RTCP_PS_Feedback_PT || packetType == RTCP_RTP_Feedback_PT) {
        processRtcpHeaders(buf,len,ssrc);
    } else {
      head->setSSRC(ssrc);
    }
  }

  void WebRtcConnection::onTransportData(char* buf, int len, Transport *transport) {
    if (audioSink_ == NULL && videoSink_ == NULL && fbSink_==NULL){
      return;
    }
    int length = len;

    // PROCESS RTCP
    if (isRTCP(buf))
      thisStats_.processRtcpPacket(buf, length);

    RTCPHeader* chead = reinterpret_cast<RTCPHeader*> (buf);
    uint8_t packetType = chead->getPacketType();
    // DELIVER FEEDBACK (RR, FEEDBACK PACKETS)
    if (packetType == RTCP_Receiver_PT || packetType == RTCP_PS_Feedback_PT || packetType == RTCP_RTP_Feedback_PT){
      if (fbSink_ != NULL) {
        fbSink_->deliverFeedback(buf,length);
      }
    } else {
      // RTP or RTCP Sender Report

      // Check incoming SSRC
      unsigned int recvSSRC = 0;
      if (packetType == RTCP_Sender_PT) { //Sender Report
        ELOG_TRACE ("RTP Sender Report %d length %d ", packetType, chead->getLength());
        recvSSRC = chead->getSSRC();
      } else {
        RTPHeader* head = reinterpret_cast<RTPHeader*> (buf);
        recvSSRC = head->getSSRC();
      }

      if (bundle_) {
        // Deliver data
        if (recvSSRC==this->getVideoSourceSSRC() || recvSSRC==this->getVideoSinkSSRC()) {
          parseIncomingPayloadType(buf, len, VIDEO_PACKET);
          if (videoSink_ != NULL)
            videoSink_->deliverVideoData(buf, len);
        } else if (recvSSRC==this->getAudioSourceSSRC() || recvSSRC==this->getAudioSinkSSRC()) {
          parseIncomingPayloadType(buf, len, AUDIO_PACKET);
          if (audioSink_ != NULL)
            audioSink_->deliverAudioData(buf, len);
        } else {
          ELOG_ERROR("Unknown SSRC %u, localVideo %u, remoteVideo %u, ignoring", recvSSRC, this->getVideoSourceSSRC(), this->getVideoSinkSSRC());
        }
      } else if (transport->mediaType == AUDIO_TYPE) {
        if (audioSink_ != NULL) {
          parseIncomingPayloadType(buf, len, AUDIO_PACKET);
          // Firefox does not send SSRC in SDP
          if (this->getAudioSourceSSRC() == 0) {
            ELOG_DEBUG("Audio Source SSRC is %u", recvSSRC);
            this->setAudioSourceSSRC(recvSSRC);
            this->updateState(TRANSPORT_READY, transport);
          }
          audioSink_->deliverAudioData(buf, length);
        }
      } else if (transport->mediaType == VIDEO_TYPE) {
        if (videoSink_ != NULL) {
          parseIncomingPayloadType(buf, len, VIDEO_PACKET);
          // Firefox does not send SSRC in SDP
          if (this->getVideoSourceSSRC() == 0) {
            ELOG_DEBUG("Video Source SSRC is %u", recvSSRC);
            this->setVideoSourceSSRC(recvSSRC);
            this->updateState(TRANSPORT_READY, transport);
          }

          videoSink_->deliverVideoData(buf, length);
        }
      }
    }
  }

  int WebRtcConnection::sendFirPacket() {
    ELOG_DEBUG("Generating FIR Packet");
    sequenceNumberFIR_++; // do not increase if repetition
    int pos = 0;
    uint8_t rtcpPacket[50];
    // add full intra request indicator
    uint8_t FMT = 4;
    rtcpPacket[pos++] = (uint8_t) 0x80 + FMT;
    rtcpPacket[pos++] = (uint8_t) 206;

    //Length of 4
    rtcpPacket[pos++] = (uint8_t) 0;
    rtcpPacket[pos++] = (uint8_t) (4);

    // Add our own SSRC
    uint32_t* ptr = reinterpret_cast<uint32_t*>(rtcpPacket + pos);
    ptr[0] = htonl(this->getVideoSinkSSRC());
    pos += 4;

    rtcpPacket[pos++] = (uint8_t) 0;
    rtcpPacket[pos++] = (uint8_t) 0;
    rtcpPacket[pos++] = (uint8_t) 0;
    rtcpPacket[pos++] = (uint8_t) 0;
    // Additional Feedback Control Information (FCI)
    uint32_t* ptr2 = reinterpret_cast<uint32_t*>(rtcpPacket + pos);
    ptr2[0] = htonl(this->getVideoSourceSSRC());
    pos += 4;

    rtcpPacket[pos++] = (uint8_t) (sequenceNumberFIR_);
    rtcpPacket[pos++] = (uint8_t) 0;
    rtcpPacket[pos++] = (uint8_t) 0;
    rtcpPacket[pos++] = (uint8_t) 0;

    if (videoTransport_ != NULL) {
      videoTransport_->write((char*)rtcpPacket, pos);
    }

    return pos;
  }

  void WebRtcConnection::updateState(TransportState state, Transport * transport) {
    boost::lock_guard<boost::mutex> lock(updateStateMutex_);
    WebRTCEvent temp = globalState_;
    ELOG_INFO("Update Transport State %s to %d", transport->transport_name.c_str(), state);
    if (audioTransport_ == NULL && videoTransport_ == NULL) {
      ELOG_ERROR("Update Transport State with Transport NULL, this should not happen!");
      return;
    }

    if (state == TRANSPORT_FAILED) {
      temp = CONN_FAILED;
      //globalState_ = CONN_FAILED;
      sending_ = false;
      ELOG_INFO("WebRtcConnection failed, stopping sending");
      cond_.notify_one();
      ELOG_INFO("WebRtcConnection failed, stopped sending");
    }

    
    if (globalState_ == CONN_FAILED) {
      // if current state is failed -> noop
      return;
    }

    if (state == TRANSPORT_STARTED &&
        (audioTransport_ == NULL || audioTransport_->getTransportState() == TRANSPORT_STARTED) &&
        (videoTransport_ == NULL || videoTransport_->getTransportState() == TRANSPORT_STARTED)) {
      temp = CONN_STARTED;
    }

    if (state == TRANSPORT_READY &&
        (audioTransport_ == NULL || audioTransport_->getTransportState() == TRANSPORT_READY) &&
        (videoTransport_ == NULL || videoTransport_->getTransportState() == TRANSPORT_READY) &&
        (remoteSdp_.audioDirection <= RECVONLY || this->getAudioSourceSSRC() != 0) &&
        (remoteSdp_.videoDirection <= RECVONLY || this->getVideoSourceSSRC() != 0)) {
        // WebRTCConnection will be ready only when all channels are ready,
        // and the source SSRCs are correctly configured if this connection
        // is receiving audio or video packets from the browser.
        temp = CONN_READY;
    }

    if (temp == CONN_READY && globalState_ != temp) {
      ELOG_INFO("Ready to send and receive media");
    }

    if (audioTransport_ != NULL && videoTransport_ != NULL) {
      ELOG_INFO("%s - Update Transport State end, %d - %d, %d - %d, %d - %d", 
        transport->transport_name.c_str(),
        (int)audioTransport_->getTransportState(), 
        (int)videoTransport_->getTransportState(), 
        this->getAudioSourceSSRC(),
        this->getVideoSourceSSRC(),
        (int)temp, 
        (int)globalState_);
    }
    
    if (temp == globalState_ || (temp == CONN_STARTED && globalState_ == CONN_READY))
      return;

    globalState_ = temp;
    if (connEventListener_ != NULL)
      connEventListener_->notifyEvent(globalState_);
  }

  // changes the outgoing payload type for in the given data packet
  void WebRtcConnection::changeDeliverPayloadType(dataPacket *dp, packetType type) {
    RTPHeader* h = reinterpret_cast<RTPHeader*>(dp->data);
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(dp->data);
    uint8_t packetType = chead->getPacketType();
    if (packetType != RTCP_Sender_PT && packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT) {
        int internalPT = h->getPayloadType();
        int externalPT = internalPT;
        if (type == AUDIO_PACKET) {
            externalPT = remoteSdp_.getAudioExternalPT(internalPT);
        } else if (type == VIDEO_PACKET) {
            externalPT = remoteSdp_.getVideoExternalPT(externalPT);
        }

        if (internalPT == RED_90000_PT) {
          assert(type == VIDEO_PACKET);
          redheader* redhead = (redheader*)(dp->data + h->getHeaderLength());
          redhead->payloadtype = remoteSdp_.getVideoExternalPT(redhead->payloadtype);
        }

        if (internalPT != externalPT) {
            h->setPayloadType(externalPT);
        }
    }
  }

  // parses incoming payload type, replaces occurence in buf
  void WebRtcConnection::parseIncomingPayloadType(char *buf, int len, packetType type) {
      RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
      RTPHeader* h = reinterpret_cast<RTPHeader*>(buf);
      uint8_t packetType = chead->getPacketType();
      if (packetType != RTCP_Sender_PT && packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT) {
        int externalPT = h->getPayloadType();
        int internalPT = externalPT;
        if (type == AUDIO_PACKET) {
            internalPT = remoteSdp_.getAudioInternalPT(externalPT);
        } else if (type == VIDEO_PACKET) {
            internalPT = remoteSdp_.getVideoInternalPT(externalPT);
        }

        if (internalPT == RED_90000_PT) {
          assert(type == VIDEO_PACKET);
          redheader* redhead = (redheader*)(buf + h->getHeaderLength());
          redhead->payloadtype = remoteSdp_.getVideoInternalPT(redhead->payloadtype);
        }

        if (externalPT != internalPT) {
            h->setPayloadType(internalPT);
            //ELOG_ERROR("onTransportData mapping %i to %i", externalPT, internalPT);
        } else {
            //ELOG_ERROR("onTransportData did not find mapping for %i", externalPT);
        }
      }
  }


  void WebRtcConnection::queueData(int comp, const char* buf, int length, Transport *transport, packetType type) {
    if ((audioSink_ == NULL && videoSink_ == NULL && fbSink_==NULL) || !sending_) //we don't enqueue data if there is nothing to receive it
      return;

    if (length > MAX_DATA_PACKET_SIZE) {
      ELOG_WARN("Discarding the data packet as the length %d exceeds the allowed maximum packet size.", length);
      return;
    }

    if (!sending_)
      return;
    if (comp == -1){
      sending_ = false;
      std::queue<dataPacket> empty;
      boost::mutex::scoped_lock lock(receiveMediaMutex_);
      std::swap(sendQueue_, empty);
      dataPacket p_;
      p_.comp = -1;
      p_.length = 0;
      p_.type = OTHER_PACKET;
      sendQueue_.push(p_);
      cond_.notify_one();
      return;
    }
    if (sendQueue_.size() < 1000) {
      dataPacket p_;
      memcpy(p_.data, buf, length);
      p_.comp = comp;
      p_.type = (transport->mediaType == VIDEO_TYPE) ? VIDEO_PACKET : AUDIO_PACKET;
      p_.length = length;
      changeDeliverPayloadType(&p_, type);
      boost::lock_guard<boost::mutex> lock(receiveMediaMutex_);
      sendQueue_.push(p_);
    }
    cond_.notify_one();
  }

  WebRTCEvent WebRtcConnection::getCurrentState() {
    return globalState_;
  }

  std::string WebRtcConnection::getJSONStats(){
    return thisStats_.getStats();
  }

  void WebRtcConnection::processRtcpHeaders(char* buf, int len, unsigned int ssrc){
    char* movingBuf = buf;
    int rtcpLength = 0;
    int totalLength = 0;
    ELOG_TRACE("RTCP PACKET");
    do {
      movingBuf += rtcpLength;
      RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(movingBuf);
      rtcpLength = (chead->getLength() + 1) * 4;      
      totalLength += rtcpLength;
      chead->setSSRC(ssrc);

      switch (chead->getPacketType()) {
        case RTCP_Receiver_PT: {
          int blockOffset = sizeof(RTCPHeader);
          while (blockOffset < rtcpLength) {
            ReportBlock* report = reinterpret_cast<ReportBlock*>(movingBuf + blockOffset);
            if (report->getSourceSSRC() == getVideoSinkSSRC()) {
              ELOG_TRACE("Rewrite video source SSRC in the RTCP report block from %u to %u", getVideoSinkSSRC(), getVideoSourceSSRC());
              report->setSourceSSRC(getVideoSourceSSRC());
            } else if (report->getSourceSSRC() == getAudioSinkSSRC()) {
              ELOG_TRACE("Rewrite audio source SSRC in the RTCP report block from %u to %u", getAudioSinkSSRC(), getAudioSourceSSRC());
              report->setSourceSSRC(getAudioSourceSSRC());
            } else {
              ELOG_TRACE("source SSRC %u in the RTCP report block unchanged", report->getSourceSSRC());
            }
            blockOffset += sizeof(ReportBlock);
          }
          break;
        }
        case RTCP_PS_Feedback_PT: {
          ELOG_TRACE("Payload specific Feedback packet %d", chead->getRCOrFMT());
          if (chead->getRCOrFMT() == 4) // It is a FIR Packet, we generate it
            this->sendFirPacket();
          break;
        }
        case RTCP_RTP_Feedback_PT:
        case RTCP_Sender_PT:
        default:
          break;
      }

    } while (totalLength < len);
  }

  void WebRtcConnection::sendLoop() {
      while (sending_) {
          dataPacket p;
          {
              boost::unique_lock<boost::mutex> lock(receiveMediaMutex_);
              while (sendQueue_.size() == 0) {
                  cond_.wait(lock);
                  if (!sending_) {
                      return;
                  }
              }
              if (sendQueue_.front().comp ==-1) {
                  sending_ = false;
                  ELOG_DEBUG("Finishing send Thread, packet -1");
                  sendQueue_.pop();
                  return;
              }

              p = sendQueue_.front();
              sendQueue_.pop();
          }

          if (bundle_ || p.type == VIDEO_PACKET) {
              videoTransport_->write(p.data, p.length);
          } else {
              audioTransport_->write(p.data, p.length);
          }
      }
  }
}
/* namespace erizo */
