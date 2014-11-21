/* <COPYRIGHT_TAG> */
#ifndef MEDIA_TYPES_H
#define MEDIA_TYPES_H

typedef enum {
    ELEMENT_MODE_ACTIVE,
    ELEMENT_MODE_PASSIVE
} ElementMode;

typedef struct MediaBuf{
    //for trace
    unsigned long mDecStartTime;
    unsigned long mIsFirstFrame;

    unsigned long mDecOutTime;
    unsigned long mDecToVpp;
    unsigned long mVppTime;
    unsigned long mVppToEnc;
    unsigned long mEncTime;
    unsigned long mDecToEnc;

    unsigned short mWidth;
    unsigned short mHeight;
    void* msdk_surface;
    void* ext_opaque_alloc;

    unsigned is_key_frame;

    // audio element
    unsigned char* payload;
    unsigned int payload_length;
    bool isFirstPacket;
    int  is_eos;

    MediaBuf():
    is_eos(0) {
    };
} MediaBuf;
#endif
