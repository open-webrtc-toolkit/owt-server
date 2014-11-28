#ifndef VideoMixEngine_h
#define VideoMixEngine_h


#include <map>

#include "msdk_xcoder.h"


#define INVALID_INPUT_INDEX -1
#define INVALID_OUTPUT_INDEX INVALID_INPUT_INDEX

typedef unsigned int BgColor;
typedef int InputIndex;
typedef int OutputIndex;

class VideoInputProducer {
public:
    virtual void requestKeyFrame(InputIndex index) = 0; 
};

class VideoOutputConsumer {
public:
    virtual void notifyFrameReady(OutputIndex index) = 0;
};

typedef struct {
    CodecType codec;
    VideoInputProducer* producer;
    void* decHandle;
    MemPool* mp;
} InputInfo;

typedef struct {
    CodecType codec;
    VideoOutputConsumer* consumer;
    const char* name; //char name[64];
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
    enum PipelineState{UN_INITIALIZED = 0,
                       IDLE,
                       WAITING_FOR_INPUT,
                       WAITING_FOR_OUTPUT,
                       IN_SERVICE};
public:
    VideoMixEngine();
    virtual ~VideoMixEngine();

    bool Init(BgColor bgColor, unsigned int width, unsigned int height);

    void SetBackgroundColor(BgColor bgColor);
    void SetResolution(unsigned int width, unsigned int height);
    void SetLayout(LayoutInfo& layout);

    InputIndex EnableInput(CodecType codec, VideoInputProducer* producer);
    void DisableInput(InputIndex index);
    void PushInput(InputIndex index, unsigned char* data, int len);

    OutputIndex EnableOutput(CodecType codec, const char* name, unsigned short bitrate, VideoOutputConsumer* consumer);
    void DisableOutput(OutputIndex index);
    void RequestKeyFrame(OutputIndex index);
    void SetBitrate(OutputIndex index, unsigned short bitrate);
    int PullOutput(OutputIndex index, unsigned char* buf);

private:
    InputIndex scheduleInput(CodecType codec, VideoInputProducer* producer);
    void installInput(InputIndex index);
    void uninstallInput(InputIndex index);
    void removeInput(InputIndex index);

    OutputIndex scheduleOutput(CodecType codec, const char* name, unsigned short bitrate, VideoOutputConsumer* consumer);
    void installOutput(OutputIndex index);
    void uninstallOutput(OutputIndex index);
    void removeOutput(OutputIndex index);

    void setupPipeline();
    void demolishPipeline();

    bool isCodecAlreadyInUse(CodecType codec);

private:
    PipelineState m_state;
    int m_inputIndex;
    int m_outputIndex;
    MsdkXcoder* m_xcoder;
    VppInfo* m_vpp;
    std::map<InputIndex, InputInfo> m_inputs;
    std::map<OutputIndex, OutputInfo> m_outputs;
};



#endif
