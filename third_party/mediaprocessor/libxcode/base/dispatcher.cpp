/* <COPYRIGHT_TAG> */
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "dispatcher.h"

Dispatcher::Dispatcher()
{
}

Dispatcher::~Dispatcher()
{
}

bool Dispatcher::Init(void *cfg, ElementMode mode)
{
    element_mode_ = mode;
    return true;
}

int Dispatcher::Recycle(MediaBuf &buf)
{
    std::map<void*, unsigned int>::iterator iter_map;
    
    Locker<Mutex> lock(surMutex);
    if (buf.msdk_surface == NULL || sur_ref_map_.empty()) {
        return 0;
    }

    iter_map = sur_ref_map_.find(buf.msdk_surface);

    if (iter_map == sur_ref_map_.end()) {
        printf("Dispatcher %p got an invalid msdk_surface %p, shall not happen\n", this, buf.msdk_surface);
        assert(0);
    }

    sur_ref_map_[buf.msdk_surface]--;

    // printf("---- Recycle: Dispatcher %p got msdk_surface 0x%x, count %d\n", this, buf.msdk_surface, sur_ref_map_[buf.msdk_surface]);
    if (sur_ref_map_[buf.msdk_surface] == 0) {
        sur_ref_map_.erase(buf.msdk_surface);
        // printf("---- Dispatcher %p, msdk_surface 0x%x refcount is 0, return it back\n", this, buf.msdk_surface);
        ReturnBuf(buf);
    }

    return 0;
}

int Dispatcher::HandleProcess()
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

        if (buf.msdk_surface != NULL) {
            Locker<Mutex> lock(surMutex);
            sur_ref_map_[buf.msdk_surface] = 0;
        }

        ProcessChain(sinkpad, buf);

        if (buf.msdk_surface == NULL) {
            printf("Dispatcher %p got EOF\n", this);
            break;
        }
    }

    return 0;
}

int Dispatcher::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    std::list<MediaPad *>::iterator it_srcpad;

    WaitSrcMutex();
    for (it_srcpad = srcpads_.begin(); it_srcpad != srcpads_.end(); it_srcpad++) {
        if ((*it_srcpad)->get_pad_status() == MEDIA_PAD_LINKED) {
            Locker<Mutex> lock(surMutex);
            if (buf.msdk_surface != NULL) {
                sur_ref_map_[buf.msdk_surface]++;
            }
        }
    }

    for (it_srcpad = srcpads_.begin(); it_srcpad != srcpads_.end(); it_srcpad++) {
        //printf("---- Dispatcher %p push msdk_surface 0x%x to element 0x%x\n",
        //        this, buf.msdk_surface,
        //        (*it_srcpad)->get_peer_pad()->get_parent());
        (*it_srcpad)->PushBufToPeerPad(buf);
    }
    ReleaseSrcMutex();

    //do not need to return buf immediately, so return -1
    return -1;
}
