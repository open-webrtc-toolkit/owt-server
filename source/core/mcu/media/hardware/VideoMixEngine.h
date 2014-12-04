#ifndef VideoMixEngine_h
#define VideoMixEngine_h


#include <map>

#include "msdk_xcoder.h"


#define INVALID_INPUT_INDEX -1
#define INVALID_OUTPUT_INDEX INVALID_INPUT_INDEX

typedef int InputIndex;
typedef int OutputIndex;

class VideoMixEngineInput {
public:
    virtual void requestKeyFrame(InputIndex index) = 0; 
};

class VideoMixEngineOutput {
public:
    virtual void notifyFrameReady(OutputIndex index) = 0;
};

/* background color*/
typedef struct {
    union {
        unsigned short Y;
        unsigned short R;
    };
    union {
        unsigned short U;
        unsigned short G;
    };
    union {
        unsigned short V;
        unsigned short B;
    };
} BgColor;

typedef struct {
    CodecType codec;
    VideoMixEngineInput* producer;
    void* decHandle;
    MemPool* mp;
} InputInfo;

typedef struct {
    CodecType codec;
    VideoMixEngineOutput* consumer;
    unsigned short bitrate;
    void* encHandle;
    Stream* stream;
} OutputInfo;

typedef struct {
    BgColor bgColor;
    unsigned int width;
    unsigned int height;
    void* vppHandle;
} VppInfo;

typedef struct {

} LayoutInfo;

class VideoMixEngine {
    typedef enum {
        UN_INITIALIZED = 0,
        IDLE,
        WAITING_FOR_INPUT,
        WAITING_FOR_OUTPUT,
        IN_SERVICE
    } PipelineState;

public:
    VideoMixEngine();
    virtual ~VideoMixEngine();

    bool Init(BgColor bgColor, unsigned int width, unsigned int height);

    void SetBackgroundColor(BgColor bgColor);
    void SetResolution(unsigned int width, unsigned int height);
    void SetLayout(LayoutInfo& layout);

    InputIndex EnableInput(CodecType codec, VideoMixEngineInput* producer);
    void DisableInput(InputIndex index);
    void PushInput(InputIndex index, unsigned char* data, int len);

    OutputIndex EnableOutput(CodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer);
    void DisableOutput(OutputIndex index);
    void ForceKeyFrame(OutputIndex index);
    void SetBitrate(OutputIndex index, unsigned short bitrate);
    int PullOutput(OutputIndex index, unsigned char* buf);

private:
    InputIndex scheduleInput(CodecType codec, VideoMixEngineInput* producer);
    void installInput(InputIndex index);
    void uninstallInput(InputIndex index);
    void removeInput(InputIndex index);

    OutputIndex scheduleOutput(CodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer);
    void installOutput(OutputIndex index);
    void uninstallOutput(OutputIndex index);
    void removeOutput(OutputIndex index);

    void setupPipeline();
    void demolishPipeline();

    bool isCodecAlreadyInUse(CodecType codec);

private:
    PipelineState m_state;
    unsigned int m_inputIndex;
    unsigned int m_outputIndex;
    MsdkXcoder* m_xcoder;
    VppInfo* m_vpp;
    std::map<InputIndex, InputInfo> m_inputs;
    std::map<OutputIndex, OutputInfo> m_outputs;
};



#endif
