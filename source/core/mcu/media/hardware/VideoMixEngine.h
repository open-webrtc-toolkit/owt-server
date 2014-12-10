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

struct FrameSize {
    int width;
    int height;
};

struct InputInfo {
    CodecType codec;
    VideoMixEngineInput* producer;
    void* decHandle;
    MemPool* mp;
};

struct OutputInfo {
    CodecType codec;
    VideoMixEngineOutput* consumer;
    unsigned short bitrate;
    void* encHandle;
    Stream* stream;
};

struct VppInfo {
    BgColor bgColor;
    unsigned int width;
    unsigned int height;
    void* vppHandle;
};

struct RegionInfo {
    std::string id;
    float left; // percentage
    float top; // percentage
    float relativeSize; //fraction
    float priority;
};

struct CustomLayoutInfo {
    FrameSize rootSize;
    BgColor rootColor;

    // Valid for customized video layout - Custom type in VCSA
    std::map<InputIndex, RegionInfo> layoutMapping;
    std::vector<RegionInfo> candidateRegions;
};

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

    bool init(BgColor bgColor, unsigned int width, unsigned int height);

    void setBackgroundColor(BgColor* bgColor);
    void setResolution(unsigned int width, unsigned int height);
    void setLayout(const CustomLayoutInfo& layout);

    InputIndex enableInput(CodecType codec, VideoMixEngineInput* producer);
    void disableInput(InputIndex index);
    void pushInput(InputIndex index, unsigned char* data, int len);

    OutputIndex enableOutput(CodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer);
    void disableOutput(OutputIndex index);
    void forceKeyFrame(OutputIndex index);
    void setBitrate(OutputIndex index, unsigned short bitrate);
    int pullOutput(OutputIndex index, unsigned char* buf);

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
    Mutex m_inputs_mutex;
    Mutex m_outputs_mutex;
};

#endif
