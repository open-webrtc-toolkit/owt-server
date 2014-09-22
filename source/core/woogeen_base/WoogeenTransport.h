/*
 * WoogeenTransport.h
 *
 *  Created on: Aug 22, 2014
 *      Author: qzhang8
 */

#ifndef WOOGEENTRANSPORT_H_
#define WOOGEENTRANSPORT_H_

#include <MediaDefinitions.h>
#include <webrtc/common_types.h>

namespace woogeen_base {

class WoogeenTransport: public webrtc::Transport {
public:
	WoogeenTransport(erizo::MediaSink* sink);
	virtual ~WoogeenTransport();

    virtual int SendPacket(int channel, const void *data, int len);
    virtual int SendRTCPPacket(int channel, const void *data, int len);

private:
    erizo::MediaSink* sink_;
};

}
#endif /* WOOGEENTRANSPORT_H_ */
