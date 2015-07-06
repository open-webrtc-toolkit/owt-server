/* <COPYRIGHT_TAG> */
#include <assert.h>
#include <stdio.h>
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
#define FRMW_TRACE_INFO  printf
#define FRMW_TRACE_ERROR printf
#else
#include <unistd.h>
#include "trace.h"
#endif
#include "base_element.h"
#include "media_pad.h"

BaseElement::BaseElement()
{
    is_running_ = false;
    thread_created_ = false;
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    src_pad_mutex_ = CreateMutex(NULL, FALSE, NULL);
    sink_pad_mutex_ = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&src_pad_mutex_, NULL);
    pthread_mutex_init(&sink_pad_mutex_, NULL);
#endif
}

BaseElement::~BaseElement()
{
    std::list<MediaPad *>::iterator it_srcpad;
    std::list<MediaPad *>::iterator it_sinkpad;
    this->Stop();

    for (it_srcpad = srcpads_.begin(); it_srcpad != srcpads_.end(); it_srcpad++) {
        // printf("element %p, deleting srcpad %p\n", this, *it_srcpad);
        delete(*it_srcpad);
    }

    for (it_sinkpad = this->sinkpads_.begin();
            it_sinkpad != this->sinkpads_.end(); it_sinkpad++) {
        // printf("element %p, deleting sinkpad %p\n", this, *it_sinkpad);
        delete(*it_sinkpad);
    }

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    CloseHandle(src_pad_mutex_);
    CloseHandle(sink_pad_mutex_);
#else
    pthread_mutex_destroy(&src_pad_mutex_);
    pthread_mutex_destroy(&sink_pad_mutex_);
#endif
}

int BaseElement::RemovePad(MediaPad *pad)
{
    if (NULL == pad) {
        return -1;
    }

    PadRemoved(pad);
    // Search for lists to remove pad
    for (std::list<MediaPad*>::iterator it_srcpad = srcpads_.begin();
            it_srcpad != srcpads_.end();
            ++it_srcpad) {
        if (*it_srcpad == pad) {
            delete(*it_srcpad);
            srcpads_.erase(it_srcpad);
            return 0;
        }
    }

    for (std::list<MediaPad*>::iterator it_sinkpad = sinkpads_.begin();
            it_sinkpad != sinkpads_.end();
            ++it_sinkpad) {
        if (*it_sinkpad == pad) {
            delete(*it_sinkpad);
            sinkpads_.erase(it_sinkpad);
            return 0;
        }
    }

    return -1;
}

int BaseElement::AddPad(MediaPad *pad)
{
    if (NULL == pad) {
        return -1;
    }

    if (pad->get_pad_direction() == MEDIA_PAD_SINK) {
        this->sinkpads_.push_back(pad);
        pad->SetChainFunction(ProcessChainCallback, this);
        // FRMW_TRACE_INFO("Element %p add Sink pad %p", this, pad);
    } else if (pad->get_pad_direction() == MEDIA_PAD_SRC) {
        this->srcpads_.push_back(pad);
        pad->SetRecycleFunction(RecycleCallback, this);
        // FRMW_TRACE_INFO("Element %p add Src pad %p", this, pad);
    } else {
        return -1;
    }

    pad->set_parent(this);
    pad->set_parent_mode(element_mode_);
    NewPadAdded(pad);
    return 0;
}

// TODO: May extend the function to another class for N:1 case
// Leave it later
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
void BaseElement::ThreadFunc(void *arg)
{
    BaseElement *element = static_cast<BaseElement *>(arg);
    element->HandleProcess();
    return;
}
#else
void *BaseElement::ThreadFunc(void *arg)
{
    BaseElement *element = static_cast<BaseElement *>(arg);
    element->HandleProcess();
    return NULL;
}
#endif



#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
bool BaseElement::Start()
{
    // FRMW_TRACE_INFO("Start element %p ", this);
    // If active mode, Create the thread
    is_running_ = true;

    if (ELEMENT_MODE_ACTIVE == element_mode_) {
        // FRMW_TRACE_INFO("ELEMENT_MODE_ACTIVE\n", this);
        thread_id_ = _beginthread( ThreadFunc, 0, this);
        if(-1 != thread_id_) {
            thread_created_ = true;
            is_running_ = true;
        }
        else {
            is_running_ = false;
        }
    }

    if (!is_running_) {
        FRMW_TRACE_ERROR("Element %p start failed\n", this);
        return false;
    }

    return true;
}
#else
bool BaseElement::Start()
{
    // FRMW_TRACE_INFO("Start element %p", this);
    // If active mode, Create the thread
    is_running_ = true;

    if (ELEMENT_MODE_ACTIVE == element_mode_) {
        is_running_ = (pthread_create(&thread_id_, NULL, ThreadFunc, this) == 0);
        if(is_running_) {
            thread_created_ = true;
        }
    }

    if (!is_running_) {
        FRMW_TRACE_ERROR("Element %p start failed\n", this);
        return false;
    }

    return true;
}
#endif

bool BaseElement::Stop()
{
    is_running_ = false;
    if (ELEMENT_MODE_ACTIVE == element_mode_) {
        if(thread_created_) {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)

#else
            pthread_join(thread_id_, NULL);
            thread_created_ = false;
#endif
        }
    }
    return true;
}

int BaseElement::UnlinkPrevElement()
{
    std::list<MediaPad *>::iterator it_sinkpad;
    MediaPad *first_sink_pad = NULL;
    MediaPad *peer_src_pad = NULL;
    BaseElement* peer_element = NULL;

    for (it_sinkpad = sinkpads_.begin(); \
            it_sinkpad != sinkpads_.end();) {
        if (*it_sinkpad && (*it_sinkpad)->get_pad_status() == MEDIA_PAD_LINKED) {
            // Found the unlinked src pad
            first_sink_pad = *it_sinkpad;
            //break;
            if (first_sink_pad) {
                peer_src_pad = first_sink_pad->get_peer_pad();
                peer_element = peer_src_pad->get_parent();
                WaitSinkMutex();
                peer_element->WaitSrcMutex();
                RemovePad(first_sink_pad);
                peer_element->RemovePad(peer_src_pad);
                peer_element->ReleaseSrcMutex();
                ReleaseSinkMutex();
                it_sinkpad = sinkpads_.begin();
            } else {
                it_sinkpad = sinkpads_.erase(it_sinkpad);
            }
        } else {
            ++it_sinkpad;
        }
    }

    if (!first_sink_pad) {
        // Not linked.
        return -1;
    } else {
        return 0;
    }
}

int BaseElement::UnlinkNextElement()
{
    std::list<MediaPad *>::iterator it_srcpad;
    MediaPad *first_src_pad = NULL;
    MediaPad *peer_sink_pad = NULL;
    BaseElement* peer_element = NULL;

    for (it_srcpad = srcpads_.begin(); \
            it_srcpad != srcpads_.end();) {
        if (*it_srcpad && (*it_srcpad)->get_pad_status() == MEDIA_PAD_LINKED) {
            // Found the unlinked src pad
            first_src_pad = *it_srcpad;
            //break;
            if (first_src_pad) {
                peer_sink_pad = first_src_pad->get_peer_pad();
                peer_element = peer_sink_pad->get_parent();
                WaitSrcMutex();
                peer_element->WaitSinkMutex();
                //Remove sink_pad at first. (especially to detach decoder from VPP)
                peer_element->RemovePad(peer_sink_pad);
                RemovePad(first_src_pad);
                peer_element->ReleaseSinkMutex();
                ReleaseSrcMutex();
                it_srcpad = srcpads_.begin();
            } else {
                it_srcpad = srcpads_.erase(it_srcpad);
            }
        } else {
            ++it_srcpad;
        }
    }

    if (!first_src_pad) {
        // Not linked.
        return -1;
    } else {
        return 0;
    }
}

MediaPad* BaseElement::GetFreePad(int type)
{
    std::list<MediaPad *>::iterator it_pad;
    MediaPad *first_pad = NULL;

    if (MEDIA_PAD_SRC == type) {
        for (it_pad = srcpads_.begin(); it_pad != srcpads_.end(); it_pad++) {
            if ((*it_pad)->get_pad_status() == MEDIA_PAD_UNLINKED) {
                first_pad = *it_pad;
                break;
            }
        }

        if (NULL == first_pad) {
            // For src pad, passive mode true/false doesn't matter
            first_pad = new MediaPad(MEDIA_PAD_SRC);
            this->AddPad(first_pad);
        }
    } else if (MEDIA_PAD_SINK == type) {
        for (it_pad = sinkpads_.begin();
                it_pad != sinkpads_.end(); it_pad++) {
            if (MEDIA_PAD_UNLINKED == (*it_pad)->get_pad_status()) {
                first_pad = *it_pad;
                break;
            }
        }

        if (NULL == first_pad) {
            first_pad = new MediaPad(MEDIA_PAD_SINK);
            this->AddPad(first_pad);
        }
    } else {
        return NULL;
    }
    return first_pad;
}

// Link from the src element to the sink
// This function will look for the existing pads that are not linked yet.
// It will request new pad if needed.
int BaseElement::LinkNextElement(BaseElement *element)
{
    int retval = -1;
    MediaPad *first_sink_pad = NULL;
    MediaPad *first_src_pad = NULL;
    std::list<MediaPad *>::iterator it_srcpad;
    std::list<MediaPad *>::iterator it_sinkpad;
    if (!element) {
        return retval;
    }

    WaitSrcMutex();
    element->WaitSinkMutex();

    first_src_pad = this->GetFreePad(MEDIA_PAD_SRC);
    first_sink_pad = element->GetFreePad(MEDIA_PAD_SINK);

    if (NULL != first_src_pad &&
            NULL != first_sink_pad) {
        retval = first_src_pad->SetPeerPad(first_sink_pad);
    }
    element->ReleaseSinkMutex();
    ReleaseSrcMutex();
    return retval;
}


int BaseElement::ProcessChainCallback(MediaBuf &buf, void *arg1, void *arg2)
{
    BaseElement *element = static_cast<BaseElement *>(arg1);
    MediaPad *pad = static_cast<MediaPad *>(arg2);
    return element->ProcessChain(pad, buf);
}

int BaseElement::RecycleCallback(MediaBuf &buf, void *arg)
{
    BaseElement *element = static_cast<BaseElement *>(arg);
    return element->Recycle(buf);
}

bool BaseElement::IsAlive()
{
    return is_running_;
}

bool BaseElement::Join()
{
    if (ELEMENT_MODE_ACTIVE == element_mode_) {
        if(thread_created_) {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)

#else
            pthread_join(thread_id_, NULL);
#endif
            thread_created_ = false;
        }
    }

    return true;
}

void BaseElement::PadRemoved(MediaPad *pad)
{
    return;
}

void BaseElement::NewPadAdded(MediaPad *pad)
{
    //FRMW_TRACE_INFO("Add a new pad %p, Direction is %d, Status is %d",
    //        pad, pad->get_pad_direction(),
    //        pad->get_pad_status());
    return;
}

int BaseElement::ReturnBuf(MediaBuf &buf)
{
    // Return buf belong to its prev element
    std::list<MediaPad *>::iterator it_sinkpad;

    //don't unlink prev element when there are buffers to return
    assert(sinkpads_.size());
    for (it_sinkpad = sinkpads_.begin(); it_sinkpad != sinkpads_.end(); it_sinkpad++) {
        assert((*it_sinkpad)->get_pad_status() == MEDIA_PAD_LINKED);
        (*it_sinkpad)->ReturnBufToPeerPad(buf);
        // printf("Element 0x%x return sid 0x%x to its prev element\n", this, buf.sid);
    }

    return 0;
}

int BaseElement::HandleProcess()
{
    // 1. Get buf from sinkpad
    // 2. Process
    // 3. Return buf through sinkpad
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(sinkpad != *(this->sinkpads_.end()));

    while (is_running_) {
        if (sinkpad->GetBufData(buf) != 0) {
            // No data, just sleep and wait
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
            Sleep(10000/1000);
#else
            usleep(10000);
#endif
            continue;
        }

        ProcessChain(sinkpad, buf);
        ReturnBuf(buf);
    }

    return 0;
}

bool BaseElement::WaitSrcMutex()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    WaitForSingleObject(src_mutex_, INFINITE);
#else
    pthread_mutex_lock(&src_pad_mutex_);
#endif
    return true;
}

bool BaseElement::ReleaseSrcMutex()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    ReleaseMutex(src_pad_mutex_);
#else
    pthread_mutex_unlock(&src_pad_mutex_);
#endif
    return true;
}

bool BaseElement::WaitSinkMutex()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    WaitForSingleObject(sink_pad_mutex_, INFINITE);
#else
    pthread_mutex_lock(&sink_pad_mutex_);
#endif
    //just return
    return true;
}

bool BaseElement::ReleaseSinkMutex()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    ReleaseMutex(sink_pad_mutex_);
#else
    pthread_mutex_unlock(&sink_pad_mutex_);
#endif
    //just return
    return true;
}
