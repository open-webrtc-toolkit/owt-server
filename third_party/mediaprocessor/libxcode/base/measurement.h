/* <COPYRIGHT_TAG> */
#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

#include <stdio.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include "trace.h"
#include <vector>
#include <string>

typedef struct
{
    unsigned long start_t;
    unsigned long finish_t;
} timerec;

typedef struct
{
    unsigned int mElementType;
    std::string mInputName;
    std::string mCodecName;
    unsigned int mChannelNum;
} pipelineinfo;

typedef std::vector<timerec> timestamp;
typedef struct
{
    pipelineinfo pinfo;
    void *id;
    timestamp FrameTimeStp;
    timestamp Enduration;
    timestamp InitStp;
} codecdata;

typedef std::vector<codecdata> codecgroup;

enum StampType {
    ENC_FRAME_TIME_STAMP = 1, //record time cost of each frame
    VPP_FRAME_TIME_STAMP,
    DEC_FRAME_TIME_STAMP,
    ENC_ENDURATION_TIME_STAMP, //the enduration of the element
    VPP_ENDURATION_TIME_STAMP, //the enduration of the element
    DEC_ENDURATION_TIME_STAMP, //the enduration of the element
    ENC_INIT_TIME_STAMP,
    VPP_INIT_TIME_STAMP,
    DEC_INIT_TIME_STAMP
};

enum MeasurementError {
    MEASUREMNT_ERROR_NONE = 0,
    MEASUREMNT_ERROR_CODECDATA,
    MEASUREMNT_FINISHSTP_NOT_SET
};

class Measurement
{
public:
    Measurement();
    virtual ~Measurement();

    void StartTime();
    void EndTime();
    void EndTime(unsigned long *time);
    void GetCurTime(unsigned long *time);
    void GetLock();
    void RelLock();
    //void    Log(unsigned char level, const char* fmt, ...);

    unsigned int FrameTime()         {
        return frame_time_;
    };
    unsigned int ChannelCodecTime()  {
        return channel_codec_time_;
    };

    void UpdateCodecNum()    {
        ++channel_codec_num_;
    };
    void UpdateWholeNum()    {
        ++whole_num_;
    };
    unsigned int ChannelCodecNum()   {
        return channel_codec_num_;
    };
    unsigned int WholeNum()          {
        return whole_num_;
    };

    unsigned long CalcInterval(timerec *tr);
    codecdata *GetCodecData(StampType st, void *id);

    int TimeStpStart(StampType st, void *hdl);

    int TimeStpFinish(StampType st, void *hdl);

    void SetElementInfo(StampType st, void *hdl, pipelineinfo *pif);

    //show the performance data, or do something user wanted with the data
    void ShowPerformanceInfo();

protected:
private :
    void GetFmtTime(char *time);
    struct timeval ttime_;
    /*codec time*/
    unsigned long frame_time_; /*codec time for one frame*/
    unsigned long channel_codec_time_;/*codec time for one stream*/
    /*codec num*/
    unsigned int channel_codec_num_;/*codec num for one stream*/
    static unsigned int whole_num_;/*a total num for all stream*/
    /*performance data*/
    codecgroup mEncGrp;
    codecgroup mDecGrp;
    codecgroup mVppGrp;
    pthread_mutex_t mutex_bch;

    static unsigned int cnt_bench, sum;
    static unsigned long init_enc, init_vpp, init_dec;
};
#endif /* _TRACE_H_*/
