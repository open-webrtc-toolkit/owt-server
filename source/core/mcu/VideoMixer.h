/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef VIDEOMIXER_H_
#define VIDEOMIXER_H_

#include <map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <OneToManyProcessor.h>


namespace woogeen_base {
class ProtectedRTPReceiver;
class ProtectedRTPSender;
class WoogeenTransport;
}

namespace erizo {
class WebRtcConnection;
}

namespace mcu {
class BufferManager;
class VCMOutputProcessor;

/**
 * Represents a Many to Many connection.
 * Receives media from several publishers, mixed into one stream and retransmits it to every subscriber.
 * By design,a subscriber could be another OneToManyProcessor, which can be used to implement many to many
 * use cases.
 */
class VideoMixer : public erizo::MediaSink, public erizo::MediaSource {
	DECLARE_LOGGER();
public:

	VideoMixer();
	virtual ~VideoMixer();
	virtual bool init();
	/**
	 * Add a Publisher.
	 * Each publisher will be served by a InputProcessor, which is responsible for
	 * decoding the incoming streams into I420Frames
	 * @param webRtcConn The WebRtcConnection of the Publisher
	 */
	void addPublisher(erizo::MediaSource* puber);
	/**
	 * Sets the subscriber
	 * @param webRtcConn The WebRtcConnection of the subscriber
	 * @param peerId An unique Id for the subscriber
	 */
	void addSubscriber(erizo::MediaSink* suber, const std::string& peerId);
	/**
	 * Eliminates the subscriber
	 * @param puber
	 */
	void removeSubscriber(const std::string& peerId);

	void removePublisher(erizo::MediaSource* puber);

	/**
	 * called by WebRtcConnections' onTransportData. This VideoMixer
	 * will be set as the MediaSink of all the WebRtcConnections in the
	 * same room
	 */
	virtual int deliverAudioData(char* buf, int len, MediaSource* from);
	/**
	 * called by WebRtcConnections' onTransportData. This VideoMixer
	 * will be set as the MediaSink of all the WebRtcConnections in the
	 * same room
	 */
	virtual int deliverVideoData(char* buf, int len, MediaSource* from);

  	// implement MediaSource
  	virtual int sendFirPacket() { return 0; }

  	void closeSink();


	/**
	 * Closes all the subscribers and the publisher, the object is useless after this
	 */
	void closeAll();

	int assignSlot(erizo::MediaSource* pub) {
		for (int i = 0; i < puberSlotMap_.size(); i++) {
			if (puberSlotMap_[i] == NULL) {
				puberSlotMap_[i] = pub;
				return i;
			}
		}
		puberSlotMap_.push_back(pub);
		return puberSlotMap_.size() - 1;
	}

	int maxSlot() {
		return puberSlotMap_.size();
	}

	// find the slot number for the corresponding puber
	// return -1 if not found
	int getSlot(erizo::MediaSource* pub) {
		for (int i = 0; i < puberSlotMap_.size(); i++) {
			if (puberSlotMap_[i] == pub)
				return i;
		}
		return -1;

	}

private:


	erizo::OneToManyProcessor* subscriber_;
	std::map<erizo::MediaSource*, woogeen_base::ProtectedRTPReceiver*> publishers_;
	std::vector<erizo::MediaSource*> puberSlotMap_;	// each publisher will be allocated one index
	VCMOutputProcessor* op_;
	woogeen_base::WoogeenTransport* transport_;
	BufferManager* bufferManager_;

};

} /* namespace mcu */
#endif /* VIDEOMIXER_H_ */
