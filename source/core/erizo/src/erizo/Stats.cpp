/*
 * Stats.cpp
 *
 */

#include <sstream>

#include "Stats.h"
#include "WebRtcConnection.h"

namespace erizo {

  DEFINE_LOGGER(Stats, "Stats");
  Stats::~Stats(){
    if (runningStats_){
      runningStats_ = false;
      statsThread_.join();
      ELOG_DEBUG("Stopped periodic stats report");
    }
  }
  void Stats::processRtcpPacket(char* buf, int length) {
    boost::mutex::scoped_lock lock(mapMutex_);    
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
  }
  
  void Stats::processRtcpPacket(RTCPHeader* chead) {    
    unsigned int ssrc = chead->getSSRC();
    uint8_t packetType = chead->getPacketType();
    uint32_t rtcpLength= (chead->getLength()+1)*4;      
    ELOG_DEBUG("RTCP Packet: PT %d, SSRC %u, RC/FMT %d ", packetType, ssrc, chead->getRCOrFMT()); 
    if (packetType == RTCP_Receiver_PT){
      char* movingBuf = reinterpret_cast<char*>(chead);
      // NEED to consider multiple report blocks in a single RTCP RR!
      uint32_t blockOffset = sizeof(RTCPHeader);
      while (blockOffset < rtcpLength) {
          ReportBlock* report = reinterpret_cast<ReportBlock*>(movingBuf + blockOffset);
          uint32_t sourceSSRC = report->getSourceSSRC();
          // TODO: This "add" fraction lost is totally WRONG, or MEANINGLESS!
          addFragmentLost(report->getFractionLost(), sourceSSRC);
          setPacketsLost(report->getCumulativeLost(), sourceSSRC);
          setJitter(report->getJitter(), sourceSSRC);
          blockOffset += sizeof(ReportBlock);
      }

    }else if (packetType == RTCP_Sender_PT){
      SenderReport* sr = reinterpret_cast<SenderReport*>(chead);
      setRtcpPacketSent(sr->getPacketCount(), ssrc);
      setRtcpBytesSent(sr->getOctetCount(), ssrc);
    }else if (packetType == 206){
      // ELOG_DEBUG("REMB packet mantissa %u, exp %u", chead->getBrMantis(), chead->getBrExp());
    }
  }
 
  std::string Stats::getStats() {
    boost::mutex::scoped_lock lock(mapMutex_);
    std::ostringstream theString;
    theString << "[";
    for (fullStatsMap_t::iterator itssrc=theStats_.begin(); itssrc!=theStats_.end();){
      unsigned long int currentSSRC = itssrc->first;
      theString << "{\"ssrc\":\"" << currentSSRC << "\",\n";
        for (singleSSRCstatsMap_t::iterator it=theStats_[currentSSRC].begin(); it!=theStats_[currentSSRC].end();){
          theString << "\"" << it->first << "\":\"" << it->second << "\"";
          if (++it != theStats_[currentSSRC].end()){
            theString << ",\n";
          }          
        }
        theString << "}";
        if (++itssrc != theStats_.end()){
          theString << ",";
        }
      }
    theString << "]";
    return theString.str(); 
  }

  void Stats::setPeriodicStats(int intervalMillis, WebRtcConnectionStatsListener* listener) {
    if (!runningStats_){
      theListener_ = listener;
      iterationsPerTick_ = static_cast<int>((intervalMillis*1000)/SLEEP_INTERVAL_);
      runningStats_ = true;
      ELOG_DEBUG("Starting periodic stats report with interval %d, iterationsPerTick %d", intervalMillis, iterationsPerTick_);
      statsThread_ = boost::thread(&Stats::sendStats, this);
    }else{
      ELOG_ERROR("Stats already started");
    }
  }

  void Stats::sendStats() {
    while(runningStats_) {
      if (++currentIterations_ >= (iterationsPerTick_)){
        theListener_->notifyStats(this->getStats());

        currentIterations_ =0;
      }
      usleep(SLEEP_INTERVAL_);
    }
  }
}

