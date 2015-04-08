#ifndef VideoMixEngine_h
#define VideoMixEngine_h

#include <string>
#include <map>
#include <vector>

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
    unsigned int width;
    unsigned int height;
};

enum VideoMixCodecType {
    VCT_MIX_VP8 = 0,
    VCT_MIX_H264,
    VCT_MIX_UNKNOWN
};

/* background color*/
struct BackgroundColor {
    unsigned short y;
    unsigned short cb;
    unsigned short cr;
};

struct RegionInfo {
    float left;            //"x / WIDTH"
    float top;             //"y / HEIGHT"
    float width_ratio;     //"width / WIDTH"
    float height_ratio;    //"height / HEIGHT"
};

typedef std::map<InputIndex, RegionInfo> CustomLayoutInfo;

class VideoMixEngineImp;
class VideoMixEngine {
public:
    VideoMixEngine();
    virtual ~VideoMixEngine();

    bool init(BackgroundColor bgColor, FrameSize frameSize);

    void setBackgroundColor(const BackgroundColor* bgColor);
    void setResolution(unsigned int width, unsigned int height);
    void setLayout(const CustomLayoutInfo* layout);

    InputIndex enableInput(VideoMixCodecType codec, VideoMixEngineInput* producer);
    void disableInput(InputIndex index);
    void pushInput(InputIndex index, unsigned char* data, int len);

    OutputIndex enableOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer);
    void disableOutput(OutputIndex index);
    void forceKeyFrame(OutputIndex index);
    void setBitrate(OutputIndex index, unsigned short bitrate);
    int pullOutput(OutputIndex index, unsigned char* buf);

private:
    VideoMixEngineImp* m_imp;
};

#endif
