#include "ExternalInput.h"

#include <arpa/inet.h>
#include <boost/cstdint.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstdio>
#include <rtputils.h>
#include <sys/time.h>

namespace erizo {
  const uint32_t BUFFER_SIZE = 2*1024*1024;
  DEFINE_LOGGER(ExternalInput, "media.ExternalInput");
  /*
  options: {
    url: 'xxx',
    transport: 'tcp'/'udp',
    buffer_size: 1024*1024*4
  }
  */
  ExternalInput::ExternalInput(const std::string& options)
      : transport_opts_(NULL)
      , running_(false)
      , context_(NULL)
      , timeoutHandler_(NULL)
      , audio_sequence_number_(0)
      , statusListener_(NULL) {
    boost::property_tree::ptree pt;
    std::istringstream is(options);
    boost::property_tree::read_json(is, pt);
    url_ = pt.get<std::string>("url", "");
    std::string transport = pt.get<std::string>("transport", "udp");
    if (transport.compare("tcp") == 0) {
      av_dict_set(&transport_opts_, "rtsp_transport", "tcp", 0);
      ELOG_DEBUG("url: %s, transport::tcp", url_.c_str());
    } else {
      char buf[256];
      uint32_t buffer_size = pt.get<uint32_t>("buffer_size", BUFFER_SIZE);
      snprintf(buf, sizeof(buf), "%u", buffer_size);
      av_dict_set(&transport_opts_, "buffer_size", buf, 0);
      ELOG_DEBUG("url: %s, transport::%s, buffer_size: %u", url_.c_str(), transport.c_str(), buffer_size);
    }
    videoDataType_ = DataContentType::ENCODED_FRAME;
    audioDataType_ = DataContentType::RTP;
  }

  ExternalInput::~ExternalInput(){
    ELOG_DEBUG("closing %s" , url_.c_str());
    running_ = false;
    thread_.join();
    av_free_packet(&avpacket_);
    if (context_!=NULL)
      avformat_free_context(context_);
    if (timeoutHandler_!=NULL) {
      delete timeoutHandler_;
      timeoutHandler_ = NULL;
    }
    av_dict_free(&transport_opts_);
    ELOG_DEBUG("closed");
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
    timeoutHandler_ = new TimeoutHandler(20000);
    context_->interrupt_callback = {&TimeoutHandler::check_interrupt, timeoutHandler_};
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
    //open rtsp
    av_init_packet(&avpacket_);
    avpacket_.data = NULL;
    int res = avformat_open_input(&context_, url_.c_str(), NULL, &transport_opts_);
    char errbuff[500];
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

      int videoCodecId = st->codec->codec_id;
      if(videoCodecId == AV_CODEC_ID_VP8 || videoCodecId == AV_CODEC_ID_H264) {
        if (!videoSourceSSRC_) {
          unsigned int videoSourceId = rand();
          ELOG_DEBUG("Set video SSRC : %d ", videoSourceId);
          setVideoSourceSSRC(videoSourceId);
        }
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

      int audioCodecId = audio_st->codec->codec_id;
      if (audioCodecId == AV_CODEC_ID_PCM_MULAW ||
          audioCodecId == AV_CODEC_ID_PCM_ALAW ||
          audioCodecId == AV_CODEC_ID_ADPCM_G722 ||
          audioCodecId == AV_CODEC_ID_OPUS) {
        if (!audioSourceSSRC_) {
          unsigned int audioSourceId = rand();
          ELOG_DEBUG("Set audio SSRC : %d", audioSourceId);
          setAudioSourceSSRC(audioSourceId);
        }
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

    ELOG_DEBUG("Start playing %s", url_.c_str() );
    while (running_) {
      timeoutHandler_->reset(1000);
      if (av_read_frame(context_, &avpacket_)<0) {
        // Try to re-open the input - silently.
        timeoutHandler_->reset(10000);
        av_read_pause(context_);
        avformat_close_input(&context_);
        ELOG_WARN("Read input data failed; trying to reopen input from url %s", url_.c_str());
        timeoutHandler_->reset(10000);
        int res = avformat_open_input(&context_, url_.c_str(), NULL, &transport_opts_);
        char errbuff[500];
        if(res != 0){
          av_strerror(res, (char*)(&errbuff), 500);
          ELOG_ERROR("Error opening input %s", errbuff);
          running_ = false;
          return;
        }
        av_read_play(context_);
        continue;
      }

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
      av_free_packet(&avpacket_);
      av_init_packet(&avpacket_);
    }
    running_=false;
    av_read_pause(context_);
  }
}
