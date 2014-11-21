/* <COPYRIGHT_TAG> */
#ifndef TEE_H
#define TEE_H

#include <list>
#include <map>

#include "base_element.h"
#include "media_types.h"
#include "media_pad.h"
#include "mutex.h"

// An 1 to N class
class Dispatcher: public BaseElement
{
public:
    Dispatcher();
    ~Dispatcher();

    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual int Recycle(MediaBuf &buf);

    virtual int HandleProcess();

    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

private:
    std::map<void*, unsigned int> sur_ref_map_;

    Mutex surMutex;
    // TODO:
    // Dynamic add/remove session, may need mutex
};
#endif

