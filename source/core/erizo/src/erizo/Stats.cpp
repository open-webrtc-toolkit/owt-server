/*
 * Stats.cpp
 *
 */

#include <sstream>

#include "Stats.h"
#include "WebRtcConnection.h"

namespace erizo {

  DEFINE_LOGGER(Stats, "Stats");
  
  Stats::Stats(){ 
    ELOG_DEBUG("Constructor Stats");
    theListener_ = NULL;
  }
  Stats::~Stats(){
    ELOG_DEBUG("Destructor Stats");
  }

  void Stats::processRtcpPacket(char* buf, int length) {
    boost::recursive_mutex::scoped_lock lock(mapMutex_);
    char* movingBuf = buf;
    int rtcpLength = 0;
    int totalLength = 0;
    
    do{
      movingBuf+=rtcpLength;
      RTCPHeader *chead= reinterpret_cast<RTCPHeader*>(movingBuf);
      rtcpLength= (chead->getLength()+1)*4;      
      totalLength+= rtcpLength;
      this->processRtcpPacket(chead);
    } while(totalLength<length);
    sendStats();
  }
  
  void Stats::processRtcpPacket(RTCPHeader* chead) {    
    unsigned int ssrc = chead->getSSRC();
    uint8_t packetType = chead->getPacketType();

    ELOG_DEBUG("RTCP SubPacket: PT %d, SSRC %u,  block count %d ",packetType,ssrc, chead->getRCOrFMT()); 
    switch(packetType){
      case RTCP_SDES_PT:
        ELOG_DEBUG("SDES");
        break;
      case RTCP_Receiver_PT: {
        uint32_t rtcpLength= (chead->getLength()+1)*4;      
        char* movingBuf = reinterpret_cast<char*>(chead);
        // NEED to consider multiple report blocks in a single RTCP RR!
        uint32_t blockOffset = sizeof(RTCPHeader);
        while (blockOffset < rtcpLength) {
          ReportBlock* report = reinterpret_cast<ReportBlock*>(movingBuf + blockOffset);
          uint32_t sourceSSRC = report->getSourceSSRC();
          // TODO: This "add" fraction lost is totally WRONG, or MEANINGLESS!
          setFractionLost(report->getFractionLost(), sourceSSRC);
          setPacketsLost(report->getCumulativeLost(), sourceSSRC);
          setJitter(report->getJitter(), sourceSSRC);
          blockOffset += sizeof(ReportBlock);
        }
        break;
      }
      case RTCP_Sender_PT: {
        SenderReport* sr = reinterpret_cast<SenderReport*>(chead);
        setRtcpPacketSent(sr->getPacketCount(), ssrc);
        setRtcpBytesSent(sr->getOctetCount(), ssrc);
        break;
      }
      case RTCP_RTP_Feedback_PT:
        ELOG_DEBUG("RTP FB: Usually NACKs: %u", chead->getRCOrFMT());
        accountNACKMessage(ssrc);
        break;
      case RTCP_PS_Feedback_PT:
        ELOG_DEBUG("RTCP PS FB TYPE: %u", chead->getRCOrFMT() );
        switch(chead->getRCOrFMT()){
          case RTCP_PLI_FMT:
            ELOG_DEBUG("PLI Message");
            accountPLIMessage(ssrc);
            break;
          case RTCP_SLI_FMT:
            ELOG_DEBUG("SLI Message");
            accountSLIMessage(ssrc);
            break;
          case RTCP_FIR_FMT:
            ELOG_DEBUG("FIR Message");
            accountFIRMessage(ssrc);
            break;
          case RTCP_AFB:
            ELOG_DEBUG("AFB Message, possibly REMB");
            break;
          default:
            ELOG_WARN("Unsupported RTCP_PS FB TYPE %u",chead->getRCOrFMT());
            break;
        }
        break;
      default:
        ELOG_DEBUG("Unknown RTCP Packet, %d", packetType);
        break;
    }
  }
 
  std::string Stats::getStats() {
    boost::recursive_mutex::scoped_lock lock(mapMutex_);
    std::ostringstream theString;
    theString << "[";
    for (fullStatsMap_t::iterator itssrc=statsPacket_.begin(); itssrc!=statsPacket_.end();){
      unsigned long int currentSSRC = itssrc->first;
      theString << "{\"ssrc\":\"" << currentSSRC << "\",\n";
      if (currentSSRC == videoSSRC_){
        theString << "\"type\":\"" << "video\",\n";
      }else if (currentSSRC == audioSSRC_){
        theString << "\"type\":\"" << "audio\",\n";
      }
      for (singleSSRCstatsMap_t::iterator it=statsPacket_[currentSSRC].begin(); it!=statsPacket_[currentSSRC].end();){
        theString << "\"" << it->first << "\":\"" << it->second << "\"";
        if (++it != statsPacket_[currentSSRC].end()){
          theString << ",\n";
        }          
      }
      theString << "}";
      if (++itssrc != statsPacket_.end()){
        theString << ",";
      }
    }
    theString << "]";
    return theString.str(); 
  }
  
  void Stats::sendStats() {
    if(theListener_!=NULL)
      theListener_->notifyStats(this->getStats());
  }
}

