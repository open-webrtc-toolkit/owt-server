/* <COPYRIGHT_TAG> */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

template<typename Element, unsigned int Size = 16>
class RingBuffer
{
private:
    Element array[Size];
    unsigned int head_;
    unsigned int tail_;

public:
    RingBuffer(): head_(0), tail_(0) {}
    virtual ~RingBuffer() {}

    unsigned int DataCount() {
        if (tail_ >= head_) {
            return (tail_ - head_);
        } else {
            return (tail_ + Size - head_);
        }
    }

    bool Push(Element &item) {
        unsigned int next_tail = (tail_ + 1) % Size;

        if (next_tail != head_) {
            array[tail_] = item;
            tail_ = next_tail;
            return true;
        }

        // queue was full
        return false;
    }

    bool Pop(Element &item) {
        if (head_ == tail_) {
            return false;
        }

        item = array[head_];
        head_ = (head_ + 1) % Size;
        return true;
    }

    // Get the element, no advance
    bool Get(Element &item) {
        if (head_ == tail_) {
            return false;
        }

        item = array[head_];
        return true;
    }

    bool IsFull() {
        return ((tail_ + 1) % Size == head_);
    }

    bool IsEmpty() {
        return (head_ == tail_);
    }
};

#endif
