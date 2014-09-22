
/*
 * RtpSource.cpp
 *
 *  Created on: Aug 2, 2012
 *      Author: pedro
 */

#include "RtpSource.h"
using boost::asio::ip::udp;

namespace erizo {
  DEFINE_LOGGER(RtpSource, "RtpSource");

  RtpSource::RtpSource(const int mediaPort, const std::string& feedbackDir, 
      const std::string& feedbackPort){
    socket_.reset(new udp::socket(io_service_, 
          udp::endpoint(udp::v4(), 
            mediaPort)));
    resolver_.reset(new udp::resolver(io_service_));
    fbSocket_.reset(new udp::socket(io_service_, udp::endpoint(udp::v4(), 0)));
    query_.reset(new udp::resolver::query(udp::v4(), feedbackDir.c_str(), feedbackPort.c_str()));
    iterator_ = resolver_->resolve(*query_);
    boost::asio::ip::udp::endpoint sender_endpoint;
    socket_->async_receive_from(boost::asio::buffer(buffer_, LENGTH), sender_endpoint, 
        boost::bind(&RtpSource::handleReceive, this, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    rtpSource_thread_= boost::thread(&RtpSource::eventLoop,this);
  }

  RtpSource::~RtpSource() {
    io_service_.stop();
    rtpSource_thread_.join();

  }

  int RtpSource::deliverFeedback_(char* buf, int len){
    fbSocket_->send_to(boost::asio::buffer(buf, len), *iterator_);
    return len;

  }

  void RtpSource::handleReceive(const::boost::system::error_code& error, 
      size_t bytes_recvd) {
    if (bytes_recvd>0&&this->videoSink_){
      this->videoSink_->deliverVideoData((char*)buffer_, (int)bytes_recvd);
    }
  }
  void RtpSource::eventLoop() {
    io_service_.run();
  }

} /* namespace erizo */
