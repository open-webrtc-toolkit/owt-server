/*
 * OneToManyProcessor.h
 */

#ifndef ONETOMANYPROCESSOR_H_
#define ONETOMANYPROCESSOR_H_

#include <map>
#include <string>

#include "MediaDefinitions.h"
#include "media/ExternalOutput.h"
#include "logger.h"

namespace erizo{

class WebRtcConnection;

/**
 * Represents a One to Many connection.
 * Receives media from one publisher and retransmits it to every subscriber.
 */
class OneToManyProcessor : public MediaSink, public FeedbackSink {
	DECLARE_LOGGER();

public:
	std::map<std::string, boost::shared_ptr<MediaSink>> subscribers;
  boost::shared_ptr<MediaSource> publisher;

	OneToManyProcessor();
	virtual ~OneToManyProcessor();
	/**
	 * Sets the Publisher
	 * @param webRtcConn The WebRtcConnection of the Publisher
	 */
	void setPublisher(MediaSource* webRtcConn);
	/**
	 * Sets the subscriber
	 * @param webRtcConn The WebRtcConnection of the subscriber
	 * @param peerId An unique Id for the subscriber
	 */
	void addSubscriber(MediaSink* webRtcConn, const std::string& peerId);
	/**
	 * Eliminates the subscriber given its peer id
	 * @param peerId the peerId
	 */
	void removeSubscriber(const std::string& peerId);

	int deliverAudioData(char* buf, int len, MediaSource* from){
	    boost::mutex::scoped_lock myMonitor_;
	    return this->deliverAudioData_(buf, len, from);
 	}

	int deliverVideoData(char* buf, int len, MediaSource* from){
	    boost::mutex::scoped_lock myMonitor_;
	    return this->deliverVideoData_(buf, len, from);
	}

	int deliverFeedback(char* buf, int len){
	    boost::mutex::scoped_lock myMonitor_;
	    return this->deliverFeedback_(buf,len);
	}

private:
  typedef boost::shared_ptr<MediaSink> sink_ptr;
	char sendVideoBuffer_[2000];
	char sendAudioBuffer_[2000];
	unsigned int sentPackets_;
  std::string rtcpReceiverPeerId_;
  FeedbackSink* feedbackSink_;
  boost::mutex myMonitor_;
	
  int deliverAudioData_(char* buf, int len, MediaSource*);
	int deliverVideoData_(char* buf, int len, MediaSource*);
  int deliverFeedback_(char* buf, int len);
  void closeAll();
};

} /* namespace erizo */
#endif /* ONETOMANYPROCESSOR_H_ */
