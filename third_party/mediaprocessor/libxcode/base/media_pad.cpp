/* <COPYRIGHT_TAG> */

#include <assert.h>
#include <stdio.h>
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "base_element.h"
#include "media_pad.h"

DEFINE_MLOGINSTANCE_CLASS(MediaPad, "MediaPad");
MediaPad::MediaPad()
{
}

MediaPad::MediaPad(MediaPadDirection direction):
    pad_direction_(direction),
    pad_status_(MEDIA_PAD_UNLINKED)
{
}

MediaPad::~MediaPad()
{
    //if assert happens, pad is unlinked/deleted before queued buffers returned.
    assert(in_buf_q_.IsEmpty());
    peer_pad_ = NULL;
    parent_ = NULL;
    pad_status_ = MEDIA_PAD_UNLINKED;
    chain_func_ = NULL;
}

int MediaPad::ProcessBuf(MediaBuf &buf)
{
    int ret = 0;

    if (ELEMENT_MODE_PASSIVE == parent_element_mode_) {
        if (chain_func_) {
            ret = chain_func_(buf, chain_func_arg_, this);
        }

        //if ret=0, need to return buf immediately
        //if ret=-1, buf can not be returned right now
        if (0 == ret) {
            ret = ReturnBufToPeerPad(buf);
        }
    } else {
        while (this->get_parent()->is_running_) {
            if (in_buf_q_.IsFull()) {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
                Sleep(10000/1000);
#else
                usleep(10000);
#endif
                // MLOG_INFO("Pad %p Q is full, sleep for 10ms and check again\n", this);
            } else {
                break;
            }
        }

        //if it's parent is stopped, return the buf.
        //this may happen when 2 MSDKCodec(Active mode) are linked.
        if (this->get_parent()->is_running_) {
            in_buf_q_.Push(buf);
        } else {
            ret = ReturnBufToPeerPad(buf);
        }

    }

    return ret;
}

int MediaPad::PushBufToPeerPad(MediaBuf &buf)
{
    int ret = 0;

    if (peer_pad_ == 0) {
        assert(0);
        return -1;
    }

    if (pad_direction_ == MEDIA_PAD_SRC) {
        ret = peer_pad_->ProcessBuf(buf);
    } else {
        MLOG_ERROR("pad direction not right\n");
        ret = -1;
    }

    return ret;
}

int MediaPad::RecycleBuf(MediaBuf &buf)
{
    int ret = 0;

    if (recycle_func_) {
        ret = recycle_func_(buf, recycle_func_arg_);
    } else {
        MLOG_ERROR("Pad %p has no recycle func registered\n", this);
    }

    return ret;
}

int MediaPad::ReturnBufToPeerPad(MediaBuf &buf)
{
    int ret = 0;

    if (peer_pad_ == 0) {
        assert(0);
        return -1;
    }

    if (pad_direction_ == MEDIA_PAD_SINK) {
        ret = peer_pad_->RecycleBuf(buf);
    } else {
        MLOG_ERROR("pad direction not right\n");
        ret = -1;
    }

    return ret;
}

int MediaPad::SetPeerPad(MediaPad *peer)
{
    assert(peer != NULL);

    if (this->pad_direction_ == peer->pad_direction_) {
        MLOG_ERROR("Two pads are same direction!\n");
        return -1;
    }

    peer_pad_ = peer;
    peer->peer_pad_ = this;
    this->set_pad_status(MEDIA_PAD_LINKED);
    peer_pad_->set_pad_status(MEDIA_PAD_LINKED);
    return 0;
}

int MediaPad::PushBufData(MediaBuf &buf)
{
    while (this->get_parent()->is_running_) {
        if (in_buf_q_.IsFull()) {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
            Sleep(10);
#else
            usleep(10 * 1000);
#endif
            MLOG_INFO("Q %p full, sleep for 10ms and check again\n", this);
        } else {
            break;
        }
    }

    if (this->get_parent()->is_running_) {
        in_buf_q_.Push(buf);
        return 0;
    } else {
        return -1;
    }
}

int MediaPad::GetBufQueueSize()
{
    return (in_buf_q_.DataCount());
}

int MediaPad::GetBufData(MediaBuf &buf)
{
    if (in_buf_q_.IsEmpty()) {
        return -1;
    }

    in_buf_q_.Pop(buf);
    return 0;
}

int MediaPad::PickBufData(MediaBuf &buf)
{
    if (in_buf_q_.IsEmpty()) {
        return -1;
    }

    in_buf_q_.Get(buf);
    return 0;
}


int MediaPad::SetChainFunction(ElementChainFunc func, void *arg)
{
    chain_func_ = func;
    chain_func_arg_ = arg;
    return 0;
}

int MediaPad::SetRecycleFunction(ElementRecycleFunc func, void *arg)
{
    recycle_func_ = func;
    recycle_func_arg_ = arg;
    return 0;
}
