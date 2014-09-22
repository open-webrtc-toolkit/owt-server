/*
 * WoogeenTransport.cpp
 *
 *  Created on: Aug 22, 2014
 *      Author: qzhang8
 */

#include "WoogeenTransport.h"

namespace woogeen_base {

WoogeenTransport::WoogeenTransport(erizo::MediaSink* sink) {
	sink_ = sink;
}

WoogeenTransport::~WoogeenTransport() {
	// TODO Auto-generated destructor stub
}

int WoogeenTransport::SendPacket(int channel, const void* data, int len) {
	sink_->deliverVideoData(reinterpret_cast<char*>(const_cast<void*>(data)), len);
	return len;	//return 0 will tell rtp_sender not able to send the packet, thus impact bitrate update
}


int WoogeenTransport::SendRTCPPacket(int channel, const void *data, int len) {

	return 0;
}

}
