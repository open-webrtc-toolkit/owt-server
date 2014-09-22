/*
 * BufferManager.h
 *
 *  Created on: Aug 30, 2014
 *      Author: qzhang8
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
