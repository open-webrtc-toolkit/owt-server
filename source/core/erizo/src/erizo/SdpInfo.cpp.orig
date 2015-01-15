/*
 * SDPProcessor.cpp
 */

#include <sstream>
#include <stdio.h>
#include <cstdlib>
#include <cstring>

#include <rtputils.h>
#include "SdpInfo.h"
#include "StringUtil.h"

using std::endl;
namespace erizo {
  DEFINE_LOGGER(SdpInfo, "SdpInfo");

  static const char *SDP_IDENTIFIER = "IntelWebRTCMCU";
  static const char *cand = "a=candidate:";
  static const char *crypto = "a=crypto:";
  static const char *group = "a=group:";
  static const char *video = "m=video";
  static const char *audio = "m=audio";
  static const char *mid = "a=mid";
  static const char *ice_user = "a=ice-ufrag";
  static const char *ice_pass = "a=ice-pwd";
  static const char *ssrctag = "a=ssrc";
  static const char *savpf = "SAVPF";
  static const char *rtpmap = "a=rtpmap:";
  static const char *rtcpmux = "a=rtcp-mux";
  static const char *fp = "a=fingerprint";
  static const char *recvonly = "a=recvonly";
  static const char *sendonly = "a=sendonly";
  static const char *sendrecv = "a=sendrecv";
  static const char *nack = "nack";

  SdpInfo::SdpInfo() {
    isBundle = false;
    videoDirection = UNSPECIFIED;
    audioDirection = UNSPECIFIED;
    isRtcpMux = false;
    isFingerprint = false;
    hasAudio = false;
    hasVideo = false;
    nackEnabled = false;
    profile = AVPF;
    audioSsrc = 0;
    videoSsrc = 0;

    ELOG_DEBUG("Generating internal RtpMap");

    RtpMap vp8;
    vp8.payloadType = VP8_90000_PT;
    vp8.encodingName = "VP8";
    vp8.clockRate = 90000;
    vp8.channels = 1;
    vp8.mediaType = VIDEO_TYPE;
    vp8.enable = true;
    internalPayloadVector_.push_back(vp8);

    RtpMap h264;
    h264.payloadType = H264_90000_PT;
    h264.encodingName = "H264";
    h264.clockRate = 90000;
    h264.channels = 1;
    h264.mediaType = VIDEO_TYPE;
    h264.enable = true;
    internalPayloadVector_.push_back(h264);

    RtpMap red;
    red.payloadType = RED_90000_PT;
    red.encodingName = "red";
    red.clockRate = 90000;
    red.channels = 1;
    red.mediaType = VIDEO_TYPE;
    red.enable = true;
    internalPayloadVector_.push_back(red);

    RtpMap ulpfec;
    ulpfec.payloadType = ULP_90000_PT;
    ulpfec.encodingName = "ulpfec";
    ulpfec.clockRate = 90000;
    ulpfec.channels = 1;
    ulpfec.mediaType = VIDEO_TYPE;
    ulpfec.enable = true;
    internalPayloadVector_.push_back(ulpfec);

    RtpMap isac16;
    isac16.payloadType = ISAC_16000_PT;
    isac16.encodingName = "ISAC";
    isac16.clockRate = 16000;
    isac16.channels = 1;
    isac16.mediaType = AUDIO_TYPE;
    isac16.enable = false;
    internalPayloadVector_.push_back(isac16);

    RtpMap isac32;
    isac32.payloadType = ISAC_32000_PT;
    isac32.encodingName = "ISAC";
    isac32.clockRate = 32000;
    isac32.channels = 1;
    isac32.mediaType = AUDIO_TYPE;
    isac32.enable = false;
    internalPayloadVector_.push_back(isac32);

    RtpMap pcmu;
    pcmu.payloadType = PCMU_8000_PT;
    pcmu.encodingName = "PCMU";
    pcmu.clockRate = 8000;
    pcmu.channels = 1;
    pcmu.mediaType = AUDIO_TYPE;
    pcmu.enable = true;
    internalPayloadVector_.push_back(pcmu);

    RtpMap opus;
    opus.payloadType = OPUS_48000_PT;
    opus.encodingName = "opus";
    opus.clockRate = 48000;
    opus.channels = 2;
    opus.mediaType = AUDIO_TYPE;
    opus.enable = false;
    internalPayloadVector_.push_back(opus);
/*
    RtpMap pcma;
    pcma.payloadType = PCMA_8000_PT;
    pcma.encodingName = "PCMA";
    pcma.clockRate = 8000;
    pcma.channels = 1;
    pcma.mediaType = AUDIO_TYPE;
    pcma.enable = false;
    internalPayloadVector_.push_back(pcma);

    RtpMap cn8;
    cn8.payloadType = CN_8000_PT;
    cn8.encodingName = "CN";
    cn8.clockRate = 8000;
    cn8.channels = 1;
    cn8.mediaType = AUDIO_TYPE;
    cn8.enable = false;
    internalPayloadVector_.push_back(cn8);

    RtpMap cn16;
    cn16.payloadType = CN_16000_PT;
    cn16.encodingName = "CN";
    cn16.clockRate = 16000;
    cn16.channels = 1;
    cn16.mediaType = AUDIO_TYPE;
    cn16.enable = false;
    internalPayloadVector_.push_back(cn16);

    RtpMap cn32;
    cn32.payloadType = CN_32000_PT;
    cn32.encodingName = "CN";
    cn32.clockRate = 32000;
    cn32.channels = 1;
    cn32.mediaType = AUDIO_TYPE;
    cn32.enable = false;
    internalPayloadVector_.push_back(cn32);

    RtpMap cn48;
    cn48.payloadType = CN_48000_PT;
    cn48.encodingName = "CN";
    cn48.clockRate = 48000;
    cn48.channels = 1;
    cn48.mediaType = AUDIO_TYPE;
    cn48.enable = false;
    internalPayloadVector_.push_back(cn48);
*/
    RtpMap telephoneevent;
    telephoneevent.payloadType = TEL_8000_PT;
    telephoneevent.encodingName = "telephone-event";
    telephoneevent.clockRate = 8000;
    telephoneevent.channels = 1;
    telephoneevent.mediaType = AUDIO_TYPE;
    telephoneevent.enable = true;
    internalPayloadVector_.push_back(telephoneevent);

  }

  SdpInfo::~SdpInfo() {
  }

  bool SdpInfo::initWithSdp(const std::string& sdp) {
    processSdp(sdp);
    return true;
  }
  void SdpInfo::addCandidate(const CandidateInfo& info) {
    candidateVector_.push_back(info);

  }

  void SdpInfo::addCrypto(const CryptoInfo& info) {
    cryptoVector_.push_back(info);
  }

  std::string SdpInfo::getSdp() {
    char msidtemp[11];
    gen_random(msidtemp,10);
    msidtemp[10] = 0;

    ELOG_DEBUG("Getting SDP");

    std::ostringstream sdp;
    sdp << "v=0\n" << "o=- 0 0 IN IP4 127.0.0.1\n";
    sdp << "s=" << SDP_IDENTIFIER << "\n";
    sdp << "t=0 0\n";

    if (isBundle) {
      sdp << "a=group:BUNDLE";
      /*
      if (this->hasAudio)
        sdp << " audio";
      if (this->hasVideo)
        sdp << " video";
        */
      for (uint8_t i = 0; i < bundleTags.size(); i++){
        sdp << " " << bundleTags[i].id;
      }
      sdp << "\n";
      sdp << "a=msid-semantic: WMS "<< msidtemp << endl;
     }
    //candidates audio
    bool printedAudio = false, printedVideo = false;

    for (unsigned int it = 0; it < candidateVector_.size(); it++) {
      const CandidateInfo& cand = candidateVector_[it];
      std::string hostType_str;
      switch (cand.hostType) {
        case HOST:
          hostType_str = "host";
          break;
        case SRFLX:
          hostType_str = "srflx";
          break;
        case PRFLX:
          hostType_str = "prflx";
          break;
        case RELAY:
          hostType_str = "relay";
          break;
        default:
          hostType_str = "host";
          break;
      }

      if (cand.mediaType == AUDIO_TYPE) {
        if (!printedAudio) {
          sdp << "m=audio " << cand.hostPort
            << " RTP/" << (profile==SAVPF?"SAVPF ":"AVPF ");// << "103 104 0 8 106 105 13 126\n"
          for (std::list<RtpMap>::iterator it = payloadVector_.begin(); it != payloadVector_.end(); ++it){
            const RtpMap& payload_info = *it;
            if (payload_info.mediaType == AUDIO_TYPE && payload_info.enable)
              sdp << payload_info.payloadType <<" ";

          }
          sdp << "\n"
            << "c=IN IP4 " << cand.hostAddress
            << endl;
          if (isRtcpMux) {
            sdp << "a=rtcp:" << candidateVector_[0].hostPort << " IN IP4 " << cand.hostAddress
                << endl;
          }
          printedAudio = true;
        }

        std::string generation = " generation 0";

        int comps = cand.componentId;
        if (isRtcpMux) {
          comps++;
        }
        for (int idx = cand.componentId; idx <= comps; idx++) {
          sdp << "a=candidate:" << cand.foundation << " " << idx
              << " " << cand.netProtocol << " " << cand.priority << " "
              << cand.hostAddress << " " << cand.hostPort << " typ "
              << hostType_str;
          if (cand.hostType == SRFLX || cand.hostType == RELAY) {
            //raddr 192.168.0.12 rport 50483
            sdp << " raddr " << cand.rAddress << " rport " << cand.rPort;
          }
          sdp << generation  << endl;
        }

        iceUsername_ = cand.username;
        icePassword_ = cand.password;
      }
    }
    //crypto audio
    if (printedAudio) {
      sdp << "a=ice-ufrag:" << iceUsername_ << endl;
      sdp << "a=ice-pwd:" << icePassword_ << endl;
      //sdp << "a=ice-options:google-ice" << endl;
      if (isFingerprint) {
        sdp << "a=fingerprint:sha-256 "<< fingerprint << endl;
      }

      switch (audioDirection) {
      case SENDONLY:
        sdp << "a=sendonly" << endl;
        break;
      case RECVONLY:
        sdp << "a=recvonly" << endl;
        break;
      default:
        sdp << "a=sendrecv" << endl;
        break;
      }

      if (bundleTags.size()>2){
        ELOG_WARN("More bundleTags than supported, expect unexpected behaviour");
      }
      for (int i = 0; i < bundleTags.size(); i++){
        if(bundleTags[i].mediaType == AUDIO_TYPE){
          sdp << "a=mid:" << bundleTags[i].id << endl;
        }
      }
      sdp << "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level" << endl;
      if (isRtcpMux)
        sdp << "a=rtcp-mux\n";
      for (unsigned int it = 0; it < cryptoVector_.size(); it++) {
        const CryptoInfo& cryp_info = cryptoVector_[it];
        if (cryp_info.mediaType == AUDIO_TYPE) {
          sdp << "a=crypto:" << cryp_info.tag << " "
            << cryp_info.cipherSuite << " " << "inline:"
            << cryp_info.keyParams << endl;
        }
      }

      for (std::list<RtpMap>::iterator it = payloadVector_.begin(); it != payloadVector_.end(); ++it){
        const RtpMap& rtp = *it;
        if (rtp.mediaType==AUDIO_TYPE && rtp.enable) {
          int payloadType = rtp.payloadType;
          if (rtp.channels>1) {
            sdp << "a=rtpmap:"<<payloadType << " " << rtp.encodingName << "/"
              << rtp.clockRate << "/" << rtp.channels << endl;
          } else {
            sdp << "a=rtpmap:"<<payloadType << " " << rtp.encodingName << "/"
              << rtp.clockRate << endl;
          }
          if(rtp.encodingName == "opus"){
            sdp << "a=fmtp:"<< payloadType<<" minptime=10\n";
          }
        }
      }
      if (audioSsrc == 0) {
        audioSsrc = 44444;
      }
      sdp << "a=maxptime:60" << endl;
      sdp << "a=ssrc:" << audioSsrc << " cname:o/i14u9pJrxRKAsu" << endl<<
        "a=ssrc:"<< audioSsrc << " msid:"<< msidtemp << " a0"<< endl<<
        "a=ssrc:"<< audioSsrc << " mslabel:"<< msidtemp << endl<<
        "a=ssrc:"<< audioSsrc << " label:" << msidtemp <<"a0"<<endl;

    }

    for (unsigned int it = 0; it < candidateVector_.size(); it++) {
      const CandidateInfo& cand = candidateVector_[it];
      std::string hostType_str;
      switch (cand.hostType) {
        case HOST:
          hostType_str = "host";
          break;
        case SRFLX:
          hostType_str = "srflx";
          break;
        case PRFLX:
          hostType_str = "prflx";
          break;
        case RELAY:
          hostType_str = "relay";
          break;
        default:
          hostType_str = "host";
          break;
      }
      if (cand.mediaType == VIDEO_TYPE) {
        if (!printedVideo) {
          sdp << "m=video " << cand.hostPort << " RTP/" << (profile==SAVPF?"SAVPF ":"AVPF "); //<<  "100 101 102 103\n"

          for (std::list<RtpMap>::iterator it = payloadVector_.begin(); it != payloadVector_.end(); ++it){
            const RtpMap& payload_info = *it;
            if (payload_info.mediaType == VIDEO_TYPE && payload_info.enable)
              sdp << payload_info.payloadType <<" ";
          }

          sdp << "\n" << "c=IN IP4 " << cand.hostAddress << endl;
          if (isRtcpMux) {
            sdp << "a=rtcp:" << candidateVector_[0].hostPort << " IN IP4 " << cand.hostAddress
                << endl;
          }
          printedVideo = true;
        }

        std::string generation = " generation 0";
        int comps = cand.componentId;
        if (isRtcpMux) {
          comps++;
        }
        for (int idx = cand.componentId; idx <= comps; idx++) {
          sdp << "a=candidate:" << cand.foundation << " " << idx
              << " " << cand.netProtocol << " " << cand.priority << " "
              << cand.hostAddress << " " << cand.hostPort << " typ "
              << hostType_str;
          if (cand.hostType == SRFLX||cand.hostType == RELAY) {
            //raddr 192.168.0.12 rport 50483
            sdp << " raddr " << cand.rAddress << " rport " << cand.rPort;
          }
          sdp << generation  << endl;
        }

        iceUsername_ = cand.username;
        icePassword_ = cand.password;
      }
    }
    //crypto video
    if (printedVideo) {
      sdp << "a=ice-ufrag:" << iceUsername_ << endl;
      sdp << "a=ice-pwd:" << icePassword_ << endl;
      //sdp << "a=ice-options:google-ice" << endl;

      if (ENABLE_RTP_TRANSMISSION_TIME_OFFSET_EXTENSION)
        sdp << "a=extmap:2 urn:ietf:params:rtp-hdrext:toffset" << endl;

      if (ENABLE_RTP_ABS_SEND_TIME_EXTENSION)
        sdp << "a=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time" << endl;

      if (isFingerprint) {
        sdp << "a=fingerprint:sha-256 "<< fingerprint << endl;
      }

      switch (videoDirection) {
      case SENDONLY:
        sdp << "a=sendonly" << endl;
        break;
      case RECVONLY:
        sdp << "a=recvonly" << endl;
        break;
      default:
        sdp << "a=sendrecv" << endl;
        break;
      }

      for (int i = 0; i < bundleTags.size(); i++){
        if(bundleTags[i].mediaType == VIDEO_TYPE){
          sdp << "a=mid:" << bundleTags[i].id << endl;
        }
      }
      if (isRtcpMux)
        sdp << "a=rtcp-mux\n";
      for (unsigned int it = 0; it < cryptoVector_.size(); it++) {
        const CryptoInfo& cryp_info = cryptoVector_[it];
        if (cryp_info.mediaType == VIDEO_TYPE) {
          sdp << "a=crypto:" << cryp_info.tag << " "
            << cryp_info.cipherSuite << " " << "inline:"
            << cryp_info.keyParams << endl;
        }
      }

      for (std::list<RtpMap>::iterator it = payloadVector_.begin(); it != payloadVector_.end(); ++it){
        const RtpMap& rtp = *it;
          if (rtp.mediaType==VIDEO_TYPE && rtp.enable)
          {
              int payloadType = rtp.payloadType;
          sdp << "a=rtpmap:"<<payloadType << " " << rtp.encodingName << "/"
              << rtp.clockRate <<"\n";
          if (rtp.encodingName == "VP8" || rtp.encodingName == "H264") {
            if (rtp.encodingName == "H264") {
              sdp << "a=fmtp:"<< payloadType<<" profile-level-id=42e01f;packetization-mode=1\n";
            }
            sdp << "a=rtcp-fb:"<< payloadType<<" ccm fir\n";
            if (nackEnabled) {
              sdp << "a=rtcp-fb:"<< payloadType<<" nack\n";
            }
            // sdp << "a=rtcp-fb:"<< rtp.payloadType<<" goog-remb\n";
          }
        }
      }
      if (videoSsrc == 0) {
        videoSsrc = 55543;
      }
      sdp << "a=ssrc:" << videoSsrc << " cname:o/i14u9pJrxRKAsu" << endl<<
        "a=ssrc:"<< videoSsrc << " msid:"<< msidtemp << " v0"<< endl<<
        "a=ssrc:"<< videoSsrc << " mslabel:"<< msidtemp << endl<<
        "a=ssrc:"<< videoSsrc << " label:" << msidtemp <<"v0"<<endl;
    }
    ELOG_DEBUG("sdp local \n %s",sdp.str().c_str());
    return sdp.str();
  }

  RtpMap* SdpInfo::getCodecByName(const std::string codecName, const unsigned int clockRate) {
    for (unsigned int it = 0; it < internalPayloadVector_.size(); it++) {
      RtpMap& rtp = internalPayloadVector_[it];
      if (rtp.encodingName == codecName && rtp.clockRate == clockRate) {
        return &rtp;
      }
    }
    return NULL;
  }

  bool SdpInfo::supportCodecByName(const std::string codecName, const unsigned int clockRate) {
    RtpMap* rtp = getCodecByName(codecName, clockRate);
    if (rtp != NULL && rtp->enable) {
      return supportPayloadType(rtp->payloadType);
    }
    return false;
  }

  int SdpInfo::forceCodecSupportByName(const std::string codecName, const unsigned int clockRate, MediaType mediaType) {
    int ret = INVALID_PT;
    for (unsigned int it = 0; it < internalPayloadVector_.size(); it++) {
      RtpMap& rtp = internalPayloadVector_[it];
      if (rtp.mediaType == mediaType) {
        if (rtp.encodingName == codecName && rtp.clockRate == clockRate)
          ret = rtp.payloadType;
        else if (mediaType == AUDIO_TYPE) {
          // Exclusively use the specified audio codec.
          rtp.enable = false;
        }
      }
    }
    return ret;
  }

  bool SdpInfo::supportPayloadType(const int payloadType) {
    if (inOutPTMap.count(payloadType) > 0) {
      for (std::list<RtpMap>::iterator it = payloadVector_.begin(); it != payloadVector_.end(); ++it){
        const RtpMap& rtp = *it;
        if (outInPTMap[rtp.payloadType] == payloadType) {
          return true;
        }
      }
    }
    return false;
  }

  bool SdpInfo::removePayloadSupport(unsigned int payloadType) {
    bool found = false;
    for (unsigned int it = 0; it < internalPayloadVector_.size(); it++) {
      RtpMap& rtp = internalPayloadVector_[it];
      if (rtp.payloadType == payloadType) {
        rtp.enable = false;
        found = true;
      }
    }

    return found;
  }

  void SdpInfo::setOfferSdp(const SdpInfo& offerSdp) {
    this->isBundle = offerSdp.isBundle;
    this->profile = offerSdp.profile;
    this->isRtcpMux = offerSdp.isRtcpMux;
    this->inOutPTMap = offerSdp.inOutPTMap;
    this->outInPTMap = offerSdp.outInPTMap;
    this->hasVideo = offerSdp.hasVideo;
    this->hasAudio = offerSdp.hasAudio;
    this->bundleTags = offerSdp.bundleTags;

    switch (offerSdp.videoDirection) {
    case SENDONLY:
      videoDirection = RECVONLY;
      break;
    case RECVONLY:
      videoDirection = SENDONLY;
      break;
    default:
      videoDirection = SENDRECV;
      break;
    }

    switch (offerSdp.audioDirection) {
    case SENDONLY:
      audioDirection = RECVONLY;
      break;
    case RECVONLY:
      audioDirection = SENDONLY;
      break;
    default:
      audioDirection = SENDRECV;
      break;
    }
  }

  bool SdpInfo::setREDSupport(bool enable) {
    bool found = false;
    for (unsigned int it = 0; it < internalPayloadVector_.size(); it++) {
      RtpMap& rtp = internalPayloadVector_[it];
      if (rtp.payloadType == RED_90000_PT) {
        rtp.enable = enable;
        found = true;
      }
    }
    return found;
  }

  bool SdpInfo::setFECSupport(bool enable) {
    bool found = false;
    for (unsigned int it = 0; it < internalPayloadVector_.size(); it++) {
      RtpMap& rtp = internalPayloadVector_[it];
      if (rtp.payloadType == ULP_90000_PT) {
        rtp.enable = enable;
        found = true;
      }
    }
    return found;
  }

  bool SdpInfo::processSdp(const std::string& sdp) {

    std::string line;
    std::istringstream iss(sdp);
    std::map<int, int> audioPayloadTypePreferences;
    std::map<int, int> videoPayloadTypePreferences;
    bool nextLineRetrieved = false;

    MediaType mtype = OTHER;

    while (nextLineRetrieved || std::getline(iss, line)) {
      nextLineRetrieved = false;

      size_t isVideo = line.find(video);
      size_t isAudio = line.find(audio);
      size_t isGroup = line.find(group);
      size_t isMid = line.find(mid);
      size_t isCand = line.find(cand);
      size_t isCrypt = line.find(crypto);
      size_t isUser = line.find(ice_user);
      size_t isPass = line.find(ice_pass);
      size_t isSsrc = line.find(ssrctag);
      size_t isSAVPF = line.find(savpf);
      size_t isRtpmap = line.find(rtpmap);
      size_t isRtcpMuxchar = line.find(rtcpmux);
      size_t isFP = line.find(fp);
      size_t isRecvonly = line.find(recvonly);
      size_t isSendonly = line.find(sendonly);
      size_t isSendrecv = line.find(sendrecv);
      size_t isNack = line.find(nack);

      ELOG_DEBUG("current line -> %s", line.c_str());

      if (isRtcpMuxchar != std::string::npos){
        isRtcpMux = true;
      }

      if (isSAVPF != std::string::npos){
        profile = SAVPF;
        ELOG_DEBUG("PROFILE %s (1 SAVPF)", line.substr(isSAVPF).c_str());
      }
      if (isFP != std::string::npos){
        std::vector<std::string> parts;

        // FIXME add error checking here
        parts = stringutil::splitOneOf(line, ":", 1);
        parts = stringutil::splitOneOf(parts[1], " ");

        fingerprint = parts[1];
        isFingerprint = true;
        ELOG_DEBUG("Fingerprint %s ", fingerprint.c_str());
      }
      if (isGroup != std::string::npos) {
        std::vector<std::string> parts;
        parts = stringutil::splitOneOf(line, ":  \r\n", 10);
        if (!parts[1].compare("BUNDLE")){
          ELOG_DEBUG("BUNDLE sdp");
          isBundle = true;
        }
        if (parts.size()>=4){
          for (unsigned int tagno=2; tagno<parts.size(); tagno++){
            ELOG_DEBUG("Adding %s to bundle vector", parts[tagno].c_str());
            BundleTag theTag(parts[tagno], OTHER);
            bundleTags.push_back(theTag);
          }
        }
      }
      if (isVideo != std::string::npos) {
        mtype = VIDEO_TYPE;
        hasVideo = true;
        if (!videoPayloadTypePreferences.empty()) {
          // We currently only support maximal one audio and one video sources in the SDP.
          continue;
        }
        // m=video 57515 RTP/SAVPF 100 116 117 96
        std::vector<std::string> pieces = stringutil::splitOneOf(line, " ");
        if (pieces.size() > 3) {
          for (size_t i = 3; i < pieces.size(); ++i) {
            int payloadType = strtoul(pieces[i].c_str(), NULL, 10);
            videoPayloadTypePreferences[payloadType] = i - 3;
          }
        }
      }
      if (isAudio != std::string::npos) {
        mtype = AUDIO_TYPE;
        hasAudio = true;
        if (!audioPayloadTypePreferences.empty()) {
          // We currently only support maximal one audio and one video sources in the SDP.
          continue;
        }
        // m=audio 57515 RTP/SAVPF 111 103 104 0 8 106 105 13 126
        std::vector<std::string> pieces = stringutil::splitOneOf(line, " ");
        if (pieces.size() > 3) {
          for (size_t i = 3; i < pieces.size(); ++i) {
            int payloadType = strtoul(pieces[i].c_str(), NULL, 10);
            audioPayloadTypePreferences[payloadType] = i - 3;
          }
        }
      }
      if (isCand != std::string::npos) {
        std::vector<std::string> pieces = stringutil::splitOneOf(line, " :");
        processCandidate(pieces, mtype);
      }
      if (isCrypt != std::string::npos) {
        CryptoInfo crypinfo;
        std::vector<std::string> cryptopiece = stringutil::splitOneOf(line, " :");

        // FIXME: add error checking here
        crypinfo.cipherSuite = cryptopiece[2];
        crypinfo.keyParams = cryptopiece[4];
        crypinfo.mediaType = mtype;
        cryptoVector_.push_back(crypinfo);
        ELOG_DEBUG("Crypto Info: %s %s %d", crypinfo.cipherSuite.c_str(),
                   crypinfo.keyParams.c_str(),
                   crypinfo.mediaType);
      }
      if (isUser != std::string::npos) {
        std::vector<std::string> parts = stringutil::splitOneOf(line, ":", 1);
        // FIXME add error checking
        iceUsername_ = parts[1];
        ELOG_DEBUG("ICE username: %s", iceUsername_.c_str());
      }
      if (isPass != std::string::npos) {
        std::vector<std::string> parts = stringutil::splitOneOf(line, ":", 1);
        // FIXME add error checking
        icePassword_ = parts[1];
        ELOG_DEBUG("ICE password: %s", icePassword_.c_str());
      }
      if (isMid!= std::string::npos){
        std::vector<std::string> parts = stringutil::splitOneOf(line, ": \r\n",4);
        if (parts.size()>=2){
          std::string thisId = parts[1];
          for (uint8_t i = 0; i < bundleTags.size(); i++){
            if (!bundleTags[i].id.compare(thisId)){
              ELOG_DEBUG("Setting tag %s to mediaType %d", thisId.c_str(), mtype);
              bundleTags[i].mediaType = mtype;
            }
          }
        }else{
          ELOG_WARN("Unexpected size of a=mid element");
        }


      }
      if (isSsrc != std::string::npos) {
        std::vector<std::string> parts = stringutil::splitOneOf(line, " :\n", 2);
        // FIXME add error checking
        if ((mtype == VIDEO_TYPE)&&(videoSsrc==0)) {
          videoSsrc = strtoul(parts[1].c_str(), NULL, 10);
          ELOG_DEBUG("video ssrc: %u", videoSsrc);
        } else if ((mtype == AUDIO_TYPE)&&(audioSsrc==0)) {
          audioSsrc = strtoul(parts[1].c_str(), NULL, 10);
          ELOG_DEBUG("audio ssrc: %u", audioSsrc);
        }
      }
      if (isSendrecv != std::string::npos) {
        if (mtype == VIDEO_TYPE)
          videoDirection = SENDRECV;
        else if (mtype == AUDIO_TYPE)
          audioDirection = SENDRECV;
      }
      if (isSendonly != std::string::npos) {
        if (mtype == VIDEO_TYPE)
          videoDirection = SENDONLY;
        else if (mtype == AUDIO_TYPE)
          audioDirection = SENDONLY;
      }
      if (isRecvonly != std::string::npos) {
        if (mtype == VIDEO_TYPE)
          videoDirection = RECVONLY;
        else if (mtype == AUDIO_TYPE)
          audioDirection = RECVONLY;
      }
      if (isNack != std::string::npos) {
        nackEnabled = true;
      }
      // a=rtpmap:PT codec_name/clock_rate
      if(isRtpmap != std::string::npos){
        std::vector<std::string> parts = stringutil::splitOneOf(line, " :/\n");
        RtpMap theMap;
        unsigned int PT = strtoul(parts[1].c_str(), NULL, 10);
        std::string codecname = parts[2];
        unsigned int clock = strtoul(parts[3].c_str(), NULL, 10);

        if (codecname == "H264") {
          if (std::getline(iss, line)) {
            nextLineRetrieved = true;
            if (line.find("a=fmtp") != std::string::npos
                && line.find(parts[1].c_str()) != std::string::npos
                && line.find("packetization-mode=1") == std::string::npos) {
              continue;
            }
          }
        }

        std::map<int, int>::iterator preferenceItor;
        if (mtype == VIDEO_TYPE) {
          preferenceItor = videoPayloadTypePreferences.find(PT);
          if (preferenceItor == videoPayloadTypePreferences.end()) {
            continue;
          }
        } else if (mtype == AUDIO_TYPE) {
          preferenceItor = audioPayloadTypePreferences.find(PT);
          if (preferenceItor == audioPayloadTypePreferences.end()) {
            continue;
          }
        }

        theMap.payloadType = PT;
        theMap.encodingName = codecname;
        theMap.clockRate = clock;
        theMap.mediaType = mtype;
        theMap.enable = true;
        if (parts.size() > 4) {
          theMap.channels = strtoul(parts[4].c_str(), NULL, 10);
        } else {
          theMap.channels = 1;
        }

        bool found = false;
        for (unsigned int it = 0; it < internalPayloadVector_.size(); it++) {
          const RtpMap& rtp = internalPayloadVector_[it];
          if (rtp.mediaType == mtype && rtp.encodingName == codecname && rtp.clockRate == clock && rtp.enable) {
            outInPTMap[PT] = rtp.payloadType;
            inOutPTMap[rtp.payloadType] = PT;
            found = true;
            ELOG_DEBUG("Mapping %s/%d:%d to %s/%d:%d", codecname.c_str(), clock, PT, rtp.encodingName.c_str(), rtp.clockRate, rtp.payloadType);
          }
        }
        if (found) {
          std::list<RtpMap>::reverse_iterator rit;
          for (rit = payloadVector_.rbegin(); rit != payloadVector_.rend(); ++rit) {
            if (rit->mediaType != mtype
                || (mtype == AUDIO_TYPE && preferenceItor->second > audioPayloadTypePreferences[rit->payloadType])
                || (mtype == VIDEO_TYPE && preferenceItor->second > videoPayloadTypePreferences[rit->payloadType])) {
              break;
            }
          }

          payloadVector_.insert(rit.base(), theMap);
        }
      }
    }

    for (unsigned int i = 0; i < candidateVector_.size(); i++) {
      CandidateInfo& c = candidateVector_[i];
      c.username = iceUsername_;
      c.password = icePassword_;
      c.isBundle = isBundle;
    }

    return true;
  }

  std::vector<CandidateInfo>& SdpInfo::getCandidateInfos() {
    return candidateVector_;
  }

  std::vector<CryptoInfo>& SdpInfo::getCryptoInfos() {
    return cryptoVector_;
  }

  std::list<RtpMap>& SdpInfo::getPayloadInfos(){
    return payloadVector_;
  }

  int SdpInfo::getAudioInternalPT(int externalPT) {
      // should use separate mapping for video and audio at the very least
      // standard requires separate mappings for each media, even!
      std::map<const int, int>::iterator found = outInPTMap.find(externalPT);
      if (found != outInPTMap.end()) {
          return found->second;
      }
      return externalPT;
  }

  int SdpInfo::getVideoInternalPT(int externalPT) {
    // WARNING
    // should use separate mapping for video and audio at the very least
    // standard requires separate mappings for each media, even!
    return getAudioInternalPT(externalPT);
  }

  int SdpInfo::getAudioExternalPT(int internalPT) {
    // should use separate mapping for video and audio at the very least
    // standard requires separate mappings for each media, even!
    std::map<const int, int>::iterator found = inOutPTMap.find(internalPT);
    if (found != inOutPTMap.end()) {
        return found->second;
    }
    return internalPT;
  }

  int SdpInfo::getVideoExternalPT(int internalPT) {
      // WARNING
      // should use separate mapping for video and audio at the very least
      // standard requires separate mappings for each media, even!
      return getAudioExternalPT(internalPT);
  }

  int SdpInfo::preferredPayloadType(MediaType mediaType) {
    std::list<RtpMap>::iterator it = payloadVector_.begin();
    for (; it != payloadVector_.end(); ++it) {
      const RtpMap& rtp = *it;
      if (rtp.mediaType == mediaType) {
        int internalPT = mediaType == VIDEO_TYPE ? getVideoInternalPT(rtp.payloadType)
            : getAudioInternalPT(rtp.payloadType);
        int externalPT = mediaType == VIDEO_TYPE ? getVideoExternalPT(internalPT)
            : getAudioExternalPT(internalPT);
        if (internalPT != RED_90000_PT && internalPT != ULP_90000_PT
            && externalPT == (int)(rtp.payloadType)) {
          return internalPT;
        }
      }
    }
    return INVALID_PT;
  }

  bool SdpInfo::processCandidate(std::vector<std::string>& pieces, MediaType mediaType) {

    CandidateInfo cand;
    static const char* types_str[] = { "host", "srflx", "prflx", "relay" };
    cand.mediaType = mediaType;
    cand.foundation = pieces[1];
    cand.componentId = (unsigned int) strtoul(pieces[2].c_str(), NULL, 10);

    cand.netProtocol = pieces[3];
    // libnice does not support tcp candidates, we ignore them
    ELOG_DEBUG("cand.netProtocol=%s", cand.netProtocol.c_str());
    if (cand.netProtocol.compare("UDP") && cand.netProtocol.compare("udp")) {
      return false;
    }
    //	a=candidate:0 1 udp 2130706432 1383 52314 typ host  generation 0
    //		        0 1 2    3            4          5     6  7    8          9
    // 
    // a=candidate:1367696781 1 udp 33562367 138. 49462 typ relay raddr 138.4 rport 53531 generation 0
    cand.priority = (unsigned int) strtoul(pieces[4].c_str(), NULL, 10);
    cand.hostAddress = pieces[5];
    cand.hostPort = (unsigned int) strtoul(pieces[6].c_str(), NULL, 10);
    if (pieces[7] != "typ") {
      return false;
    }
    unsigned int type = 1111;
    int p;
    for (p = 0; p < 4; p++) {
      if (pieces[8] == types_str[p]) {
        type = p;
      }
    }
    switch (type) {
      case 0:
        cand.hostType = HOST;
        break;
      case 1:
        cand.hostType = SRFLX;
        break;
      case 2:
        cand.hostType = PRFLX;
        break;
      case 3:
        cand.hostType = RELAY;
        break;
      default:
        cand.hostType = HOST;
        break;
    }

    ELOG_DEBUG( "Candidate Info: foundation=%s, componentId=%u, netProtocol=%s, "
                "priority=%u, hostAddress=%s, hostPort=%u, hostType=%u",
                cand.foundation.c_str(),
                cand.componentId,
                cand.netProtocol.c_str(),
                cand.priority,
                cand.hostAddress.c_str(),
                cand.hostPort,
                cand.hostType);

    if (cand.hostType == SRFLX || cand.hostType==RELAY) {
      cand.rAddress = pieces[10];
      cand.rPort = (unsigned int) strtoul(pieces[12].c_str(), NULL, 10);
      ELOG_DEBUG("Parsing raddr srlfx or relay %s, %u \n", cand.rAddress.c_str(), cand.rPort);
    }
    candidateVector_.push_back(cand);
    return true;
  }

  void SdpInfo::gen_random(char* s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
  }
}/* namespace erizo */

