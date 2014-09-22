#include "RtpPacketQueue.h"
#include "../MediaDefinitions.h"
#include <rtputils.h>
#include <cstring>

namespace erizo{

DEFINE_LOGGER(RtpPacketQueue, "rtp.RtpPacketQueue");

RtpPacketQueue::RtpPacketQueue(unsigned int max, unsigned int depth) : lastSequenceNumberGiven_(-1), max_(max), depth_(depth)
{
    if(depth_ >= max_) {
        ELOG_WARN("invalid configuration, depth_: %d, max_: %d; reset to defaults", depth_, max_);
        depth_ = erizo::DEFAULT_DEPTH;
        max_ = erizo::DEFAULT_MAX;
    }
}

RtpPacketQueue::~RtpPacketQueue(void)
{
    queue_.clear();
}

void RtpPacketQueue::pushPacket(const char *data, int length)
{
    const RTPHeader *currentHeader = reinterpret_cast<const RTPHeader*>(data);
    uint16_t currentSequenceNumber = currentHeader->getSeqNumber();

    if(lastSequenceNumberGiven_ >= 0 && (rtpSequenceLessThan(currentSequenceNumber, (uint16_t)lastSequenceNumberGiven_) || currentSequenceNumber == lastSequenceNumberGiven_)) {
        // this sequence number is less than the stuff we've already handed out, which means it's too late to be of any value.
        // TODO adaptive depth?
        ELOG_WARN("SSRC:%u, Payload: %u, discarding very late sample %d that is <= %d.  Current queue depth is %d",currentHeader->getSSRC(),currentHeader->getPayloadType(), currentSequenceNumber, lastSequenceNumberGiven_, this->getSize());
        return;
    }

    // TODO this should be a secret of the dataPacket class.  It should maintain its own memory
    // and copy stuff as necessary.
    boost::shared_ptr<dataPacket> packet(new dataPacket());
    memcpy(packet->data, data, length);
    packet->length = length;

    // let's insert this packet where it belongs in the queue.
    boost::mutex::scoped_lock lock(queueMutex_);
    std::list<boost::shared_ptr<dataPacket> >::iterator it;
    for (it=queue_.begin(); it != queue_.end(); ++it) {
        const RTPHeader *header = reinterpret_cast<const RTPHeader*>((*it)->data);
        uint16_t sequenceNumber = header->getSeqNumber();

        if (sequenceNumber == currentSequenceNumber) {
            // We already have this sequence number in the queue.
            ELOG_INFO("discarding duplicate sample %d", currentSequenceNumber);
            break;
        }

        if (this->rtpSequenceLessThan(sequenceNumber, currentSequenceNumber)) {
            queue_.insert(it, packet);
            break;
        }
    }

    if (it == queue_.end()) {
        // something old, or queue is empty.
        queue_.push_back(packet);
    }

    // Enforce our max queue size.
    while(queue_.size() > max_) {
        ELOG_DEBUG("RtpPacketQueue - Discarding a sample due to hitting MAX_SIZE");
        queue_.pop_back();  // remove oldest samples.
    }
}

// pops a packet off the queue, respecting the specified queue depth.
boost::shared_ptr<dataPacket> RtpPacketQueue::popPacket(bool ignore_depth)
{
    boost::shared_ptr<dataPacket> packet;

    boost::mutex::scoped_lock lock(queueMutex_);
    if (queue_.size() > 0) {
        if (ignore_depth || queue_.size() >= depth_) {
            packet = queue_.back();
            queue_.pop_back();
            const RTPHeader *header = reinterpret_cast<const RTPHeader*>(packet->data);
            lastSequenceNumberGiven_ = (int)header->getSeqNumber();
        }
    }

    return packet;
}

int RtpPacketQueue::getSize() {
    boost::mutex::scoped_lock lock(queueMutex_);
    return queue_.size();
}

bool RtpPacketQueue::hasData() {
    boost::mutex::scoped_lock lock(queueMutex_);
    return queue_.size() >= depth_;
}

// Implements x < y, taking into account RTP sequence number wrap
// The general idea is if there's a very large difference between
// x and y, that implies that the larger one is actually "less than"
// the smaller one.
//
// I picked 0x8000 as my "very large" threshold because it splits
// 0xffff, so it seems like a logical choice.
bool RtpPacketQueue::rtpSequenceLessThan(uint16_t x, uint16_t y) {
    int diff = y - x;
    if (diff > 0) {
        return (diff < 0x8000);
    } else if (diff < 0) {
        return (diff < -0x8000);
    } else { // diff == 0
        return false;
    }
}
} /* namespace erizo */
