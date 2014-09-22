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

#ifndef BUFFERMANAGER_H_
#define BUFFERMANAGER_H_
#include <boost/lockfree/queue.hpp>
#include <logger.h>

/**
 * Each decoder thread tries to get a FramBuffer firstly from the freeBufferList. If nothing available,
 * this means the encoder thread hasn't released any of the buffer. In this case, the decoder thread simply
 * returns without waiting for a frame available.
 * If it gets a free frame, the thread will copy the decoded frame data into the frame, and then delivers the frame
 * to the encoder thread with appropriate slot, The original decoded frame at the corresponding slot needs to be
 * released to freeBufferList at the same time.
 */

namespace webrtc {
 class I420VideoFrame;
}


namespace mcu {

#define SLOT_SIZE 16


class BufferManager {
	DECLARE_LOGGER();
public:
	BufferManager();
	virtual ~BufferManager();

	webrtc::I420VideoFrame* getFreeBuffer();
	void releaseBuffer(webrtc::I420VideoFrame* frame);

	webrtc::I420VideoFrame* getBusyBuffer(int slot);

	/**
	 * return a busy frame to the original busy slot if it is empty
	 */
	webrtc::I420VideoFrame* returnBusyBuffer(webrtc::I420VideoFrame* frame, int slot);
	/**
	 * post a free frame to the busy queue of the corresponding slot, return the
	 * original busy frame at the corresponding slot.
	 */
	webrtc::I420VideoFrame* postFreeBuffer(webrtc::I420VideoFrame* frame, int slot);
private:

	/* only works for 64bit */
	static uint64_t exchange(volatile uint64_t* ptr, uint64_t value) {
	    asm volatile("lock;"
	                 "xchgq %0, %1"
	                 : "=r"(value), "=m"(*ptr)
	                 : "0"(value), "m"(*ptr)
	                 : "memory");
	    return value;
	}

	/* only works for 64bit */
	static uint64_t cmpexchange(volatile uint64_t* dst, uint64_t xchg, uint64_t compare) {
		uint64_t value = 1;
		asm volatile( "lock; cmpxchgq %2,(%1)"
		                : "=a" (value) : "r" (dst), "r" (xchg), "0" (compare) : "memory" );
		return value;
	}

	// frames in freeQ can be used to copy decoded frame data by the decoder thread
	boost::lockfree::queue<webrtc::I420VideoFrame*, boost::lockfree::capacity<SLOT_SIZE*2>> freeQ_;
	// frames in the busyQ is ready for composition by the encoder thread
	volatile webrtc::I420VideoFrame* busyQ_[SLOT_SIZE];
};

} /* namespace mcu */
#endif /* BUFFERMANAGER_H_ */
