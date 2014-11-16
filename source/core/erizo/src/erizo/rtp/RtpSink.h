/*
 * RtpSink.h
 *
 *  Created on: Aug 2, 2012
 *      Author: pedro
 */

#ifndef RTPSINK_H_
#define RTPSINK_H_

#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <queue>

#include "../MediaDefinitions.h"
#include "../logger.h"

namespace erizo {

class RtpSink: public MediaSink, public FeedbackSource {
  DECLARE_LOGGER();
  public:
	RtpSink(const std::string& url, const std::string& port, int feedbackPort);
	virtual ~RtpSink();

    int deliverAudioData(char* buf, int len);
    int deliverVideoData(char* buf, int len);

private:

  boost::scoped_ptr<boost::asio::ip::udp::socket> socket_, fbSocket_;
  boost::scoped_ptr<boost::asio::ip::udp::resolver> resolver_;

  boost::scoped_ptr<boost::asio::ip::udp::resolver::query> query_;
	boost::asio::ip::udp::resolver::iterator iterator_;
  boost::asio::io_service io_service_;

  boost::thread send_Thread_, receive_Thread_;
	boost::condition_variable cond_;
  boost::mutex queueMutex_;
  std::queue<dataPacket> sendQueue_;
  bool sending_;

  static const int LENGTH = 1500;
  char* buffer_[LENGTH];

	int sendData(char* buffer, int len);
  void sendLoop();
  void serviceLoop();
  void handleReceive(const::boost::system::error_code& error, 
      size_t bytes_recvd);
	void queueData(const char* buffer, int len, packetType type);
};

} /* namespace erizo */
#endif /* RTPSINK_H_ */
