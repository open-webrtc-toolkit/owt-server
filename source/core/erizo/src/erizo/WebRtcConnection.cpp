/*
 * WebRTCConnection.cpp
 */

#include <cstdio>

#include "WebRtcConnection.h"
#include "DtlsTransport.h"
#include "SdesTransport.h"

#include "SdpInfo.h"
#include <rtputils.h>

namespace erizo {
  DEFINE_LOGGER(WebRtcConnection, "WebRtcConnection");

  WebRtcConnection::WebRtcConnection(bool audioEnabled, bool videoEnabled, const std::string &stunServer, int stunPort, int minPort, int maxPort, const std::string& certFile, const std::string& keyFile, const std::string& privatePasswd, uint32_t qos) {

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
    connEventListener_ = NULL;
    videoTransport_ = NULL;
    audioTransport_ = NULL;

    audioEnabled_ = audioEnabled;
    videoEnabled_ = videoEnabled;

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
      connEventListener_->notifyEvent(globalState_);
      connEventListener_ = NULL;
    }
    globalState_ = CONN_FINISHED;
    videoSink_ = NULL;
    audioSink_ = NULL;
    fbSink_ = NULL;
    delete videoTransport_;
    videoTransport_=NULL;
    delete audioTransport_;
    audioTransport_= NULL;
  }

  bool WebRtcConnection::init() {
    return true;
  }

  int WebRtcConnection::setAudioCodec(const std::string& codecName, unsigned int clockRate) {
    int payloadType = localSdp_.forceCodecSupportByName(codecName, clockRate, AUDIO_TYPE);
    if (payloadType == INVALID_PT) {
      ELOG_WARN("Fail to set audio codec %s, clock rate %u", codecName.c_str(), clockRate);
    }
    return payloadType;
  }

  int WebRtcConnection::setVideoCodec(const std::string& codecName, unsigned int clockRate) {
    int payloadType = localSdp_.forceCodecSupportByName(codecName, clockRate, VIDEO_TYPE);
    if (payloadType == INVALID_PT) {
      ELOG_WARN("Fail to set video codec %s, clock rate %u", codecName.c_str(), clockRate);
    }
    return payloadType;
  }

  bool WebRtcConnection::setRemoteSdp(const std::string &sdp) {
    ELOG_DEBUG("Set Remote SDP %s", sdp.c_str());
    remoteSdp_.initWithSdp(sdp);
    //std::vector<CryptoInfo> crypto_remote = remoteSdp_.getCryptoInfos();

    CryptoInfo cryptLocal_video;
    CryptoInfo cryptLocal_audio;
    CryptoInfo cryptRemote_video;
    CryptoInfo cryptRemote_audio;

    bool videoTransportNeeded = videoEnabled_ && remoteSdp_.videoDirection >= RECVONLY;
    bool audioTransportNeeded = audioEnabled_ && remoteSdp_.audioDirection >= RECVONLY && (!remoteSdp_.isBundle || !videoTransportNeeded);
    bundle_ = audioEnabled_ && !audioTransportNeeded;

    std::vector<RtpMap> payloadRemote = remoteSdp_.getPayloadInfos();
    localSdp_.getPayloadInfos() = remoteSdp_.getPayloadInfos();
    localSdp_.isBundle = bundle_;
    localSdp_.isRtcpMux = remoteSdp_.isRtcpMux;

    ELOG_DEBUG("Video %d videossrc %u Audio %d audio ssrc %u Bundle %d", videoEnabled_, remoteSdp_.videoSsrc, audioEnabled_, remoteSdp_.audioSsrc,  bundle_);

    ELOG_DEBUG("Setting SSRC to localSdp %u", this->getVideoSinkSSRC());
    localSdp_.videoSsrc = this->getVideoSinkSSRC();
    localSdp_.audioSsrc = this->getAudioSinkSSRC();

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
          videoTransport_ = new DtlsTransport(VIDEO_TYPE, "video", bundle_, remoteSdp_.isRtcpMux, this, stunServer_, stunPort_, minPort_, maxPort_, certFile_, keyFile_, privatePasswd_);
        }
        if (audioTransportNeeded) {
          audioTransport_ = new DtlsTransport(AUDIO_TYPE, "audio", false, remoteSdp_.isRtcpMux, this, stunServer_, stunPort_, minPort_, maxPort_, certFile_, keyFile_, privatePasswd_);
        }
      } else {
        // SDES
        std::vector<CryptoInfo> crypto_remote = remoteSdp_.getCryptoInfos();
        for (unsigned int it = 0; it < crypto_remote.size(); it++) {
          CryptoInfo cryptemp = crypto_remote[it];
          if (cryptemp.mediaType == VIDEO_TYPE
              && !cryptemp.cipherSuite.compare("AES_CM_128_HMAC_SHA1_80")) {
            videoTransport_ = new SdesTransport(VIDEO_TYPE, "video", bundle_, remoteSdp_.isRtcpMux, &cryptemp, this, stunServer_, stunPort_, minPort_, maxPort_);
          } else if (audioTransportNeeded && cryptemp.mediaType == AUDIO_TYPE
              && !cryptemp.cipherSuite.compare("AES_CM_128_HMAC_SHA1_80")) {
            audioTransport_ = new SdesTransport(AUDIO_TYPE, "audio", false, remoteSdp_.isRtcpMux, &cryptemp, this, stunServer_, stunPort_, minPort_, maxPort_);
          }
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

    return true;
  }

  std::string WebRtcConnection::getLocalSdp() {
    boost::mutex::scoped_lock lock(updateStateMutex_);
    ELOG_DEBUG("Getting SDP");
    if (videoTransport_ != NULL) {
      videoTransport_->processLocalSdp(&localSdp_);
    }
    ELOG_DEBUG("Video SDP done.");
    if (!bundle_ && audioTransport_ != NULL) {
      audioTransport_->processLocalSdp(&localSdp_);
    }
    ELOG_DEBUG("Audio SDP done.");
    localSdp_.profile = remoteSdp_.profile;
    return localSdp_.getSdp();
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

  int WebRtcConnection::deliverAudioData(char* buf, int len) {
    writeSsrc(buf, len, this->getAudioSinkSSRC());
    if (bundle_){
      if (videoTransport_ != NULL) {
        if (audioEnabled_ == true) {
          this->queueData(0, buf, len, videoTransport_);
        }
      }
    } else if (audioTransport_ != NULL) {
      if (audioEnabled_ == true) {
        this->queueData(0, buf, len, audioTransport_);
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
        this->queueData(0, buf, len, videoTransport_);
      }
    }
    return len;
  }

  int WebRtcConnection::deliverFeedback(char* buf, int len){
    // Check where to send the feedback
    RTCPFeedbackHeader* chead = reinterpret_cast<RTCPFeedbackHeader*>(buf);
    uint32_t sourceSsrc = chead->getSourceSSRC();
    ELOG_DEBUG("received Feedback type %u, ssrc %u, sourceSsrc %u",
      chead->getRTCPHeader().getPacketType(), chead->getRTCPHeader().getSSRC(), sourceSsrc);
    if (sourceSsrc == getAudioSinkSSRC() || sourceSsrc == getAudioSourceSSRC()) {
      writeSsrc(buf, len, this->getAudioSinkSSRC());
      if (audioTransport_ != NULL)
        this->queueData(0, buf, len, audioTransport_);
      else {
        assert(bundle_ && videoTransport_ != NULL);
        this->queueData(0, buf, len, videoTransport_);
      }
    } else if (sourceSsrc == getVideoSinkSSRC() || sourceSsrc == getVideoSourceSSRC()) {
      writeSsrc(buf, len, this->getVideoSinkSSRC());
      assert(videoTransport_ != NULL);
      this->queueData(0, buf, len, videoTransport_);
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

    // PROCESS STATS
    if (this->statsListener_){ // if there is no listener we dont process stats
      RTPHeader *head = reinterpret_cast<RTPHeader*> (buf);
      if (head->getPayloadType() != RED_90000_PT && head->getPayloadType() != PCMU_8000_PT)     
        thisStats_.processRtcpPacket(buf, length);
    }
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
        ELOG_DEBUG ("RTP Sender Report %d length %d ", packetType, chead->getLength());
        recvSSRC = chead->getSSRC();
      } else {
        RTPHeader* head = reinterpret_cast<RTPHeader*> (buf);
        recvSSRC = head->getSSRC();
      }

      if (bundle_) {
        // Deliver data
        if (recvSSRC==this->getVideoSourceSSRC() || recvSSRC==this->getVideoSinkSSRC()) {
          videoSink_->deliverVideoData(buf, length);
        } else if (recvSSRC==this->getAudioSourceSSRC() || recvSSRC==this->getAudioSinkSSRC()) {
          audioSink_->deliverAudioData(buf, length);
        } else {
          ELOG_ERROR("Unknown SSRC %u, localVideo %u, remoteVideo %u, ignoring", recvSSRC, this->getVideoSourceSSRC(), this->getVideoSinkSSRC());
        }
      } else if (transport->mediaType == AUDIO_TYPE) {
        if (audioSink_ != NULL) {
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
      // if current state is failed we don't use
      return;
    }

    if (state == TRANSPORT_STARTED &&
        (audioTransport_ == NULL || audioTransport_->getTransportState() == TRANSPORT_STARTED) &&
        (videoTransport_ == NULL || videoTransport_->getTransportState() == TRANSPORT_STARTED)) {
      if (videoTransport_ != NULL) {
        videoTransport_->setRemoteCandidates(remoteSdp_.getCandidateInfos());
      }
      if (audioTransport_ != NULL) {
        audioTransport_->setRemoteCandidates(remoteSdp_.getCandidateInfos());
      }
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

      // Notify the media sinks to get ready for receiving media
      if (audioSink_ != NULL) {
        if (audioTransport_ != NULL || (bundle_ && videoTransport_ != NULL))
          audioSink_->audioReady();
      }

      if (videoSink_ != NULL && videoTransport_ != NULL) {
        videoSink_->videoReady();
      }
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
    
    if (temp < 0) {
      return;
    }

    if (temp == globalState_ || (temp == CONN_STARTED && globalState_ == CONN_READY))
      return;

    globalState_ = temp;
    if (connEventListener_ != NULL)
      connEventListener_->notifyEvent(globalState_);
  }

  void WebRtcConnection::queueData(int comp, const char* buf, int length, Transport *transport) {
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
      std::swap( sendQueue_, empty);
      dataPacket p_;
      p_.comp = -1;
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
      boost::lock_guard<boost::mutex> lock(receiveMediaMutex_);
      sendQueue_.push(p_);
    }
    cond_.notify_one();
  }

  WebRTCEvent WebRtcConnection::getCurrentState() {
    return globalState_;
  }

  void WebRtcConnection::processRtcpHeaders(char* buf, int len, unsigned int ssrc){
    char* movingBuf = buf;
    int rtcpLength = 0;
    int totalLength = 0;
    ELOG_DEBUG("RTCP PACKET");
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
              ELOG_DEBUG("Rewrite video source SSRC in the RTCP report block from %u to %u", getVideoSinkSSRC(), getVideoSourceSSRC());
              report->setSourceSSRC(getVideoSourceSSRC());
            } else if (report->getSourceSSRC() == getAudioSinkSSRC()) {
              ELOG_DEBUG("Rewrite audio source SSRC in the RTCP report block from %u to %u", getAudioSinkSSRC(), getAudioSourceSSRC());
              report->setSourceSSRC(getAudioSourceSSRC());
            } else {
              ELOG_DEBUG("source SSRC %u in the RTCP report block unchanged", report->getSourceSSRC());
            }
            blockOffset += sizeof(ReportBlock);
          }
          break;
        }
        case RTCP_PS_Feedback_PT: {
          ELOG_DEBUG("Payload specific Feedback packet %d", chead->getRCOrFMT());
          if (chead->getRCOrFMT() == 4) // It is a FIR Packet, we generate it
            this->sendFirPacket();
          break;
        }
        case RTCP_RTP_Feedback_PT:
        case RTCP_Sender_PT:
        default:
          break;
      }

    } while(totalLength < len);
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
              if(sendQueue_.front().comp ==-1){
                  sending_ =  false;
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
