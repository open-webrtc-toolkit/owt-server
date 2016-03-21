/* <COPYRIGHT_TAG> */

#ifndef BASEELEMENT_H
#define BASEELEMENT_H

/**
 *\file   BaseElement.h
 *
 *\brief  The element base class declaration.
 *
 */

#include <list>
#include "media_types.h"

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

class MediaPad;

class BaseElement
{
public:
    BaseElement();
    virtual ~BaseElement();

    virtual bool Init(void *cfg, ElementMode element_mode) = 0;

    int LinkNextElement(BaseElement *element);

    int UnlinkNextElement();

    int UnlinkPrevElement();

    bool Start();

    bool Stop();

    bool Join();

    bool IsAlive();

    ElementMode get_element_mode() {
        return element_mode_;
    };

    bool is_running_;

    // the input stream name in case decoder or dispatcher.
    const char* input_name;

protected:

    bool WaitSrcMutex();
    bool ReleaseSrcMutex();

    bool WaitSinkMutex();
    bool ReleaseSinkMutex();


    ElementMode element_mode_;

    virtual void NewPadAdded(MediaPad *pad);

    virtual void PadRemoved(MediaPad *pad);

    virtual int HandleProcess();


    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf) = 0;

    virtual int Recycle(MediaBuf &buf) = 0;

    int ReturnBuf(MediaBuf &buf);

    std::list<MediaPad *> sinkpads_;
    std::list<MediaPad *> srcpads_;
private:
    MediaPad* GetFreePad(int type);
    int AddPad(MediaPad *pad);
    int RemovePad(MediaPad *pad);

    static int ProcessChainCallback(MediaBuf &buf, void *arg1, void *arg2);
    static int RecycleCallback(MediaBuf &buf, void *arg);

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    static void ThreadFunc(void *arg);
#else
    static void *ThreadFunc(void *arg);
#endif



#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
	uintptr_t thread_id_;
#else
    pthread_t thread_id_;
#endif

    bool thread_created_;

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    HANDLE src_pad_mutex_;
    HANDLE sink_pad_mutex_;
#else
    pthread_mutex_t src_pad_mutex_;
    pthread_mutex_t sink_pad_mutex_;
#endif
};

class CodecEventCallback {
public:
    CodecEventCallback(){}
    virtual ~CodecEventCallback(){}

    virtual void DecodeHeaderFailEvent(void *DecHandle = 0) = 0;
};
#endif

