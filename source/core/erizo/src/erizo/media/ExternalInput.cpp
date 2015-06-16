#include "ExternalInput.h"

#include <arpa/inet.h>
#include <boost/cstdint.hpp>
#include <cstdio>
#include <rtputils.h>
#include <sys/time.h>

namespace erizo {
  DEFINE_LOGGER(ExternalInput, "media.ExternalInput");
  ExternalInput::ExternalInput(const std::string& inputUrl)
      : url_(inputUrl)
      , audio_sequence_number_(0) {
    videoDataType_ = DataContentType::ENCODED_FRAME;
    audioDataType_ = DataContentType::RTP;
    sourcefbSink_=NULL;
    context_ = NULL;
    running_ = false;
    statusListener_ = NULL;
  }

  ExternalInput::~ExternalInput(){
    ELOG_DEBUG("Destructor ExternalInput %s" , url_.c_str());
    ELOG_DEBUG("Closing ExternalInput");
    running_ = false;
    thread_.join();
    av_free_packet(&avpacket_);
    if (context_!=NULL)
      avformat_free_context(context_);
    ELOG_DEBUG("ExternalInput closed");
  }

  void ExternalInput::setStatusListener(ExternalInputStatusListener* listener) {
    statusListener_ = listener;
  }

  int ExternalInput::init() {
    thread_ = boost::thread(&ExternalInput::receiveLoop, this);
    running_ = true;

    return 0;
  }

  bool ExternalInput::connect() {
    srand((unsigned)time(NULL));

    context_ = avformat_alloc_context();
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
    //open rtsp
    av_init_packet(&avpacket_);
    avpacket_.data = NULL;
    ELOG_DEBUG("Trying to open input from url %s", url_.c_str());
    int res = avformat_open_input(&context_, url_.c_str(),NULL,NULL);
    char errbuff[500];
    printf ("RES %d\n", res);
    if(res != 0){
      av_strerror(res, (char*)(&errbuff), 500);
      ELOG_ERROR("Error opening input %s", errbuff);
      return false;
    }
    res = avformat_find_stream_info(context_,NULL);
    if(res<0){
      av_strerror(res, (char*)(&errbuff), 500);
      ELOG_ERROR("Error finding stream info %s", errbuff);
      return false;
    }

    AVStream *st, *audio_st;
    
    int streamNo = av_find_best_stream(context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (streamNo < 0){
      ELOG_WARN("No Video stream found");
      //return streamNo;
    }else{
      video_stream_index_ = streamNo;
      st = context_->streams[streamNo];
      ELOG_DEBUG("Has video, video stream number %d. time base = %d / %d, codec type = %d ",
                  video_stream_index_,
                  st->time_base.num,
                  st->time_base.den,
                  st->codec->codec_id);
      ELOG_DEBUG("AV_CODEC_ID_VP8: %d ", AV_CODEC_ID_VP8);
      ELOG_DEBUG("AV_CODEC_ID_H264: %d ", AV_CODEC_ID_H264);
      int videoCodecId = st->codec->codec_id;
      if(videoCodecId == AV_CODEC_ID_VP8 || videoCodecId == AV_CODEC_ID_H264) {
        unsigned int videoSourceId = rand();
        ELOG_DEBUG("Set video SSRC : %d ", videoSourceId);
        setVideoSourceSSRC(videoSourceId);
        if (videoCodecId == AV_CODEC_ID_VP8) {
            videoPayloadType_ = VP8_90000_PT;
        } else if (videoCodecId == AV_CODEC_ID_H264) {
            videoPayloadType_ = H264_90000_PT;
        }
      } else {
        ELOG_WARN("Video codec %d is not supported ", st->codec->codec_id);
      }
    }

    int audioStreamNo = av_find_best_stream(context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioStreamNo < 0){
      ELOG_WARN("No Audio stream found");
      //return streamNo;
    }else{
      audio_stream_index_ = audioStreamNo;
      audio_st = context_->streams[audio_stream_index_];
      ELOG_DEBUG("Has audio, audio stream number %d. time base = %d / %d, codec type = %d ",
                  audio_stream_index_,
                  audio_st->time_base.num,
                  audio_st->time_base.den,
                  audio_st->codec->codec_id);
      ELOG_DEBUG("AV_CODEC_ID_PCM_MULAW: %d ", AV_CODEC_ID_PCM_MULAW);
      ELOG_DEBUG("AV_CODEC_ID_PCM_ALAW: %d ", AV_CODEC_ID_PCM_ALAW);
      ELOG_DEBUG("AV_CODEC_ID_ADPCM_G722: %d ", AV_CODEC_ID_ADPCM_G722);
      ELOG_DEBUG("AV_CODEC_ID_OPUS: %d ", AV_CODEC_ID_OPUS);

      audio_time_base_ = audio_st->time_base.den;
      ELOG_DEBUG("Audio Time base %d", audio_time_base_);

      int audioCodecId = audio_st->codec->codec_id;
      if (audioCodecId == AV_CODEC_ID_PCM_MULAW ||
          audioCodecId == AV_CODEC_ID_PCM_ALAW ||
          audioCodecId == AV_CODEC_ID_ADPCM_G722 ||
          audioCodecId == AV_CODEC_ID_OPUS) {
        unsigned int audioSourceId = rand();
        ELOG_DEBUG("Set audio SSRC : %d", audioSourceId);
        setAudioSourceSSRC(audioSourceId);
        if (audioCodecId == AV_CODEC_ID_PCM_MULAW) {
            audioPayloadType_ = PCMU_8000_PT;
        } else if (audioCodecId == AV_CODEC_ID_PCM_ALAW) {
            audioPayloadType_ = PCMA_8000_PT;
        } else if  (audioCodecId == AV_CODEC_ID_ADPCM_G722) {
            audioPayloadType_ = INVALID_PT;
        } else if (audioCodecId == AV_CODEC_ID_OPUS) {
            audioPayloadType_ = OPUS_48000_PT;
        }
      } else {
        ELOG_WARN("Audio codec %d is not supported ", audioCodecId);
      }
    }

    if (!videoSourceSSRC_ && !audioSourceSSRC_) {
      return false;
    }

    av_init_packet(&avpacket_);

    return true;
  }

  int ExternalInput::sendFirPacket() {
    return 0;
  }

  void ExternalInput::receiveLoop() {
    bool ret = connect();

    std::string message;
    if (ret) {
        message = "success";
    } else {
        message = "opening input url error";
    }

    if (statusListener_ != NULL) {
        statusListener_->notifyStatus(message);
    }

    if (!ret) return;

    av_read_play(context_);//play RTSP

    ELOG_DEBUG("Start playing external input %s", url_.c_str() );
    while (av_read_frame(context_, &avpacket_)>=0 && running_==true) {
      AVPacket orig_pkt = avpacket_;
      if (avpacket_.stream_index == video_stream_index_) { //packet is video
        //ELOG_DEBUG("Receive video frame packet with size %d ", avpacket_.size);
        if (videoSourceSSRC_ && videoSink_!=NULL) {
          videoSink_->deliverVideoData((char*)avpacket_.data, avpacket_.size);
        }
      } else if (avpacket_.stream_index == audio_stream_index_) { //packet is audio
        //ELOG_DEBUG("Receive audio frame packet with size %d ", avpacket_.size);
        if (audioSourceSSRC_ && audioSink_!=NULL) {
          char buf[MAX_DATA_PACKET_SIZE];
          RTPHeader* head = reinterpret_cast<RTPHeader*>(buf);
          memset(head, 0, sizeof(RTPHeader));
          head->setVersion(2);
          head->setSSRC(audioSourceSSRC_);
          head->setPayloadType(audioPayloadType_);
          head->setTimestamp(avpacket_.dts);
          head->setSeqNumber(audio_sequence_number_++);
          head->setMarker(false); // not used.
          memcpy(&buf[head->getHeaderLength()], avpacket_.data, avpacket_.size);

          audioSink_->deliverAudioData(buf, avpacket_.size + head->getHeaderLength());
        }
      }
      av_free_packet(&orig_pkt);
    }
    running_=false;
    av_read_pause(context_);
  }
}
