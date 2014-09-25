/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#include "VCMMediaProcessor.h"

#include <boost/bind.hpp>
#include <string.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_receiver.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/modules/video_coding/main/source/codec_database.h>
#include <webrtc/system_wrappers/interface/tick_util.h>
#include <webrtc/system_wrappers/interface/trace.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

class DebugRecorder {
 public:
  DebugRecorder()
      : cs_(CriticalSectionWrapper::CreateCriticalSection()), file_(NULL) {}

  ~DebugRecorder() { Stop(); }

  int Start(const char* file_name_utf8) {
    CriticalSectionScoped cs(cs_.get());
    if (file_)
      fclose(file_);
    file_ = fopen(file_name_utf8, "wb");
    if (!file_)
      return VCM_GENERAL_ERROR;
    return VCM_OK;
  }

  void Stop() {
    CriticalSectionScoped cs(cs_.get());
    if (file_) {
      fclose(file_);
      file_ = NULL;
    }
  }

  void Add(const I420VideoFrame& frame) {
    CriticalSectionScoped cs(cs_.get());
    if (file_)
      PrintI420VideoFrame(frame, file_);
  }

  int PrintI420VideoFrame(const I420VideoFrame& frame, FILE* file) {
    if (file == NULL)
      return -1;
    if (frame.IsZeroSize())
      return -1;
    for (int planeNum = 0; planeNum < kNumOfPlanes; ++planeNum) {
      int width = (planeNum ? (frame.width() + 1) / 2 : frame.width());
      int height = (planeNum ? (frame.height() + 1) / 2 : frame.height());
      PlaneType plane_type = static_cast<PlaneType>(planeNum);
      const uint8_t* plane_buffer = frame.buffer(plane_type);
      for (int y = 0; y < height; y++) {
       if (fwrite(plane_buffer, 1, width, file) !=
           static_cast<unsigned int>(width)) {
         return -1;
         }
         plane_buffer += frame.stride(plane_type);
      }
   }
   return 0;
  }

 private:
  scoped_ptr<CriticalSectionWrapper> cs_;
  FILE* file_ GUARDED_BY(cs_);
};

DEFINE_LOGGER(VCMInputProcessor, "media.VCMInputProcessor");

VCMInputProcessor::VCMInputProcessor(int index, VCMOutputProcessor* op)
{
    vcm_ = NULL;
    op_ = op;
    index_ = index;
}

VCMInputProcessor::~VCMInputProcessor()
{
    if (vcm_)
        VideoCodingModule::Destroy(vcm_), vcm_ = NULL;
}

bool VCMInputProcessor::init(BufferManager*  bufferManager)
{
    Trace::CreateTrace();
    Trace::SetTraceFile("webrtc.trace.txt");
    Trace::set_level_filter(webrtc::kTraceAll);

    bufferManager_ = bufferManager;
    vcm_ = VideoCodingModule::Create(index_);
    if (vcm_) {
        vcm_->InitializeReceiver();
        vcm_->RegisterReceiveCallback(this);
    } else
        return false;

    rtp_header_parser_.reset(RtpHeaderParser::Create());
    rtp_payload_registry_.reset(new RTPPayloadRegistry(index_, RTPPayloadStrategy::CreateStrategy(false)));
    rtp_receiver_.reset(RtpReceiver::CreateVideoReceiver(index_, Clock::GetRealTimeClock(), this, NULL,
                                rtp_payload_registry_.get()));

    //register codec
    VideoCodec video_codec;
    if (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec) != VCM_OK) {
      return false;
    }

    setReceiveVideoCodec(video_codec);

    if (video_codec.codecType != kVideoCodecRED &&
        video_codec.codecType != kVideoCodecULPFEC) {
      // Register codec type with VCM, but do not register RED or ULPFEC.
      if (vcm_->RegisterReceiveCodec(&video_codec, 1, false) != VCM_OK) {
        return true;
      }
    }
    recorder_.reset(new DebugRecorder());
    recorder_->Start("/home/qzhang8/webrtc/webrtc.frame.i420");
    return true;
}

void VCMInputProcessor::close() {
    recorder_->Stop();
    Trace::ReturnTrace();
}

void VCMInputProcessor::DecoderFunc(webrtc::VideoCodingModule* vcm,
    const boost::system::error_code& /*e*/,
    boost::asio::deadline_timer* t)
{
    vcm->Decode(0);
    t->expires_at(t->expires_at() + boost::posix_time::milliseconds(33));
    t->async_wait(boost::bind(&VCMInputProcessor::DecoderFunc, vcm,
    boost::asio::placeholders::error, t));
}

/**
 * implements webrtc::VCMReceiveCallback
 */
int32_t VCMInputProcessor::FrameToRender(I420VideoFrame& videoFrame)
{
    ELOG_DEBUG("Got decoded frame from %d\n", index_);
    op_->updateFrame(videoFrame, index_);
    return 0;
}

int32_t VCMInputProcessor::OnReceivedPayloadData(
    const uint8_t* payloadData,
    const uint16_t payloadSize,
    const WebRtcRTPHeader* rtpHeader)
{
    vcm_->IncomingPacket(payloadData, payloadSize, *rtpHeader);
    int32_t ret = vcm_->Decode(0);
    ELOG_DEBUG("OnReceivedPayloadData index= %d, decode result = %d",  index_, ret);

    return 0;
}

bool VCMInputProcessor::setReceiveVideoCodec(const VideoCodec& video_codec)
{
    int8_t old_pltype = -1;
    if (rtp_payload_registry_->ReceivePayloadType(video_codec.plName,
        kVideoPayloadTypeFrequency,
        0,
        video_codec.maxBitrate,
        &old_pltype) != -1) {
        rtp_payload_registry_->DeRegisterReceivePayload(old_pltype);
    }

    return rtp_receiver_->RegisterReceivePayload(video_codec.plName,
        video_codec.plType,
        kVideoPayloadTypeFrequency,
        0,
        video_codec.maxBitrate) == 0;
}


void VCMInputProcessor::receiveRtpData(char* rtp_packet, int rtp_packet_length, erizo::DataType type , uint32_t streamId)
{
    if (type == AUDIO) {
	if(aip_ != NULL) {
        	aip_->receiveRtpData(rtp_packet, rtp_packet_length, AUDIO, streamId);
	}
	return;
     }
    RTPHeader header;
    if (!rtp_header_parser_->Parse(reinterpret_cast<uint8_t*>(rtp_packet), rtp_packet_length, &header)) {
        ELOG_DEBUG("Incoming packet: Invalid RTP header");
        return;
    }
    int payload_length = rtp_packet_length - header.headerLength;
    header.payload_type_frequency = kVideoPayloadTypeFrequency;
    bool in_order = false; /*IsPacketInOrder(header)*/;
    rtp_payload_registry_->SetIncomingPayloadType(header);
    PayloadUnion payload_specific;
    if (!rtp_payload_registry_->GetPayloadSpecifics(header.payloadType, &payload_specific)) {
       return;
    }
    rtp_receiver_->IncomingRtpPacket(header, reinterpret_cast<uint8_t*>(rtp_packet + header.headerLength) , payload_length,
        payload_specific, in_order);
}

DEFINE_LOGGER(VCMOutputProcessor, "media.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor():layoutLock_(CriticalSectionWrapper::CreateCriticalSection())
{
    vcm_ = NULL;
    vpm_ = NULL;
    layout_.subWidth_ = 640;
    layout_.subHeight_ = 480;
    layout_.div_factor_ = 1;
    layoutNew_ = layout_;
    bufferManager_= NULL;
    composedFrame_ = NULL;
    encodingThread_.reset();
    timer_.reset();
    videoMixer_ = NULL;

    // recorder_->Start("/home/qzhang8/webrtc/webrtc.frame.i420");
    recorder_.reset(new DebugRecorder());
    recordStarted_= false;
    mockFrame_ = NULL;
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    recorder_->Stop();
    this->close();
    if (vpm_)
        VideoProcessingModule::Destroy(vpm_), vpm_= NULL;
    if (vcm_)
        VideoCodingModule::Destroy(vcm_), vcm_ = NULL;
}

bool VCMOutputProcessor::init(webrtc::Transport* transport, BufferManager* bufferManager, VideoMixer* videoMixer)
{
    videoMixer_ = videoMixer;
    bufferManager_ = bufferManager;
    vcm_ = VideoCodingModule::Create(2);
    if (vcm_) {
        vcm_->InitializeSender();
        vcm_->RegisterTransportCallback(this);
        vcm_->RegisterProtectionCallback(this);
        vcm_->EnableFrameDropper(false);
    } else
        return false;

    vpm_ = VideoProcessingModule::Create(2);
    vpm_->SetInputFrameResampleMode(webrtc::kFastRescaling);
    //vpm_->SetTargetResolution(640, 480, 30);

    RtpRtcp::Configuration configuration;
    configuration.id = 002;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    default_rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(configuration));

    VideoCodec video_codec;
    if (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec) != VCM_OK) {
      return false;
    }
    video_codec.width = 640;
    video_codec.height = 480;
    if (vcm_->RegisterSendCodec(&video_codec, 1,
                               default_rtp_rtcp_->MaxDataPayloadLength()) != 0) {
        return false;
    }
    if (default_rtp_rtcp_->RegisterSendPayload(video_codec) != 0) {
        return false;
    }

    composedFrame_ = new webrtc::I420VideoFrame();
    composedFrame_->CreateEmptyFrame(640, 480,
                                     640, 640/2, 640/2);
    memset(composedFrame_->buffer(webrtc::kYPlane), 0x00, composedFrame_->allocated_size(webrtc::kYPlane));
    memset(composedFrame_->buffer(webrtc::kUPlane), 128, composedFrame_->allocated_size(webrtc::kUPlane));
    memset(composedFrame_->buffer(webrtc::kVPlane), 128, composedFrame_->allocated_size(webrtc::kVPlane));

    recordStarted_ = false;

    timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::milliseconds(33)));
    timer_->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));

    encodingThread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));

    return true;
}

void VCMOutputProcessor::close()
{
    timer_->cancel();
    io_service_.stop();
    encodingThread_->join();
    io_service_.reset();
    encodingThread_.reset();

    vcm_->StopDebugRecording();
    if (composedFrame_) {
        delete composedFrame_, composedFrame_ = NULL;
    }
}

// A return value of false is interpreted as that the function has no
// more work to do and that the thread can be released.
void VCMOutputProcessor::layoutTimerHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        layoutFrames();
        timer_->expires_at(timer_->expires_at() + boost::posix_time::milliseconds(33));
        timer_->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));
    } else {
        ELOG_INFO("VCMOutputProcessor timer error: %s", ec.message().c_str());
    }
}

bool VCMOutputProcessor::setSendVideoCodec(const VideoCodec& video_codec)
{
    return true;
}

void VCMOutputProcessor::updateMaxSlot(int newMaxSlot)
{
//    CriticalSectionScoped cs(layoutLock_.get());
    Layout layout;
    if (newMaxSlot == 1) {
        layout.div_factor_ = 1;
    } else if (newMaxSlot <= 4) {
        layout.div_factor_ = 2;
    } else if (newMaxSlot <= 9) {
        layout.div_factor_ = 3;
    } else layout.div_factor_ = 4;

    layout.subWidth_ = 640/layout.div_factor_;
    layout.subHeight_ = 480/layout.div_factor_;

    layoutNew_ = layout;    //atomic

    vpm_->SetTargetResolution(layoutNew_.subWidth_, layoutNew_.subHeight_, 30);
    ELOG_DEBUG("div_factor is changed to %d,  subWidth is %d, subHeight is %d", layoutNew_.div_factor_, layoutNew_.subWidth_, layoutNew_.subHeight_);
}
#define SCALE_BY_OUTPUT 1
#define SCALE_BY_INPUT 0
/**
 * this should be called whenever a new frame is decoded from
 * one particular publisher with index
 */
bool posted = false;
void VCMOutputProcessor::updateFrame(webrtc::I420VideoFrame& frame, int index)
{
#if SCALE_BY_OUTPUT
    I420VideoFrame* freeFrame = bufferManager_->getFreeBuffer();
    if (freeFrame == NULL)
        return;    //no free frame, simply ignore it
    else {
        freeFrame->CopyFrame(frame);
        I420VideoFrame* busyFrame = bufferManager_->postFreeBuffer(freeFrame, index);
        if (busyFrame) {
            ELOG_DEBUG("updateFrame: returning busy frame, index is %d", index);
            bufferManager_->releaseBuffer(busyFrame);
        }
        posted = true;
    }
#elif SCALE_BY_INPUT
    I420VideoFrame* processedFrame;
    int ret = vpm_->PreprocessFrame(frame, &processedFrame);
    if (ret == VPM_OK && processedFrame != NULL) {
        I420VideoFrame* freeFrame = bufferManager_->getFreeBuffer();
        if (freeFrame == NULL)
            return;
        else {
            freeFrame->CopyFrame(*processedFrame);
            I420VideoFrame* busyFrame = bufferManager_->postFreeBuffer(freeFrame, index);
               if (busyFrame)
                   bufferManager_->releaseBuffer(busyFrame);
        }
    }
#else
//    CriticalSectionScoped cs(cs_.get());
    composedFrame_->CopyFrame(frame);
    if (mockFrame_ == NULL) {
        mockFrame_ = new webrtc::I420VideoFrame();
        mockFrame_->CreateEmptyFrame(640, 480,
                                     640, 640/2, 640/2);
        mockFrame_->CopyFrame(frame);
    }
#endif

}

bool VCMOutputProcessor::layoutFrames()
{
#if SCALE_BY_OUTPUT
    webrtc::I420VideoFrame* target = composedFrame_;
    for (int input = 0; input < videoMixer_->maxSlot(); input++) {
        if ((input == 0) && !(layout_ == layoutNew_)) {
            // commit new layout config at the beginning of each iteration
            layout_ = layoutNew_;
            vpm_->SetTargetResolution(layout_.subWidth_, layout_.subHeight_, 30);
        }
        unsigned int offset_width = (input%layout_.div_factor_) * layout_.subWidth_;
        unsigned int offset_height = (input/layout_.div_factor_) * layout_.subHeight_;
        webrtc::I420VideoFrame* sub_image = bufferManager_->getBusyBuffer(input);
        if (sub_image == NULL) {
            for (int i = 0; i < layout_.subHeight_; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    layout_.subWidth_);
            }

            for (int i = 0; i < layout_.subHeight_/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
            }
            continue;
        }
        // do the scale first
        else {
            I420VideoFrame* processedFrame;
            int ret = vpm_->PreprocessFrame(*sub_image, &processedFrame);
            if (ret == VPM_OK && processedFrame == NULL) {
                // do nothing
            } else if (ret == VPM_OK && processedFrame != NULL) {
                sub_image->CopyFrame(*processedFrame);
            }

            for (int i = 0; i < layout_.subHeight_; i++) {
                memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                    sub_image->buffer(webrtc::kYPlane) + i * sub_image->stride(webrtc::kYPlane),
                    layout_.subWidth_);
            }

            for (int i = 0; i < layout_.subHeight_/2; i++) {
                memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    sub_image->buffer(webrtc::kUPlane) + i * sub_image->stride(webrtc::kUPlane),
                    layout_.subWidth_/2);
                memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    sub_image->buffer(webrtc::kVPlane) + i * sub_image->stride(webrtc::kVPlane),
                    layout_.subWidth_/2);
            }
        }
        // if return busy frame failed, which means a new busy frame has been posted
        // simply release the busy frame
        if (bufferManager_->returnBusyBuffer(sub_image, input)) {
            bufferManager_->releaseBuffer(sub_image);
            ELOG_DEBUG("releasing busyFrame[%d]", input);
        }

        if (recordStarted_ == false) {
            vcm_->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
            recordStarted_ = true;
        }
    }

    composedFrame_->set_render_time_ms(TickTime::MillisecondTimestamp());
    const int kMsToRtpTimestamp = 90;
    const uint32_t time_stamp =
        kMsToRtpTimestamp *
        static_cast<uint32_t>(composedFrame_->render_time_ms());
    composedFrame_->set_timestamp(time_stamp);
    vcm_->AddVideoFrame(*composedFrame_);
#elif SCALE_BY_INPUT
//    CriticalSectionScoped cs(layoutLock_.get());
    webrtc::I420VideoFrame* target = composedFrame_;
    for (int input = 0; input < videoMixer_->maxSlot(); input++) {
        if (input == 0) {
            // commit new layout config at the beginning of each iteration
            layout_ = layoutNew_;
        }
        unsigned int offset_width = (input%layout_.div_factor_) * layout_.subWidth_;
        unsigned int offset_height = (input/layout_.div_factor_) * layout_.subHeight_;
        webrtc::I420VideoFrame* sub_image = bufferManager_->getBusyBuffer(input);
        if (sub_image == NULL) {
            for (int i = 0; i < layout_.subHeight_; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    layout_.subWidth_);
            }

            for (int i = 0; i < layout_.subHeight_/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
            }
            continue;
        }
        // check whether the layout should be changed
        if (sub_image->width() != layout_.subWidth_ || sub_image->height() != layout_.subHeight_) {
            bufferManager_->releaseBuffer(sub_image);
            ELOG_DEBUG("busyFrame[%d] is returned due to layout change", input);
        }
        for (int i = 0; i < layout_.subHeight_; i++) {
            memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                sub_image->buffer(webrtc::kYPlane) + i * sub_image->stride(webrtc::kYPlane),
                layout_.subWidth_);
        }

        for (int i = 0; i < layout_.subHeight_/2; i++) {
            memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                sub_image->buffer(webrtc::kUPlane) + i * sub_image->stride(webrtc::kUPlane),
                layout_.subWidth_/2);
            memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                sub_image->buffer(webrtc::kVPlane) + i * sub_image->stride(webrtc::kVPlane),
                layout_.subWidth_/2);
        }
        // if return busy frame failed, which means a new busy frame has been posted
        // simply release the busy frame
        if (bufferManager_->returnBusyBuffer(sub_image, input))
            bufferManager_->releaseBuffer(sub_image);

        if (recordStarted_ == false) {
            vcm_->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
            recordStarted_ = true;
        }
    }

    vcm_->AddVideoFrame(*composedFrame_);
#else
    if (recordStarted_ == true) {
        vcm_->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
        recordStarted_ = true;
    }
    if (mockFrame_)
        vcm_->AddVideoFrame(*mockFrame_);
#endif
    return true;
}

int32_t VCMOutputProcessor::SendData(
                                    FrameType frameType,
                                    uint8_t payloadType,
                                    uint32_t timeStamp,
                                    int64_t capture_time_ms,
                                    const uint8_t* payloadData,
                                    uint32_t payloadSize,
                                    const RTPFragmentationHeader& fragmentationHeader,
                                    const RTPVideoHeader* rtpVideoHdr)
{
      // New encoded data, hand over to the rtp module.
      return default_rtp_rtcp_->SendOutgoingData(frameType,
                                                 payloadType,
                                                 timeStamp,
                                                 capture_time_ms,
                                                 payloadData,
                                                 payloadSize,
                                                 &fragmentationHeader,
                                                 rtpVideoHdr);

}

int VCMOutputProcessor::ProtectionRequest(
                                    const FecProtectionParams* delta_fec_params,
                                    const FecProtectionParams* key_fec_params,
                                    uint32_t* sent_video_rate_bps,
                                    uint32_t* sent_nack_rate_bps,
                                    uint32_t* sent_fec_rate_bps)
{
    ELOG_DEBUG("ProtectionRequest");
    default_rtp_rtcp_->SetFecParameters(delta_fec_params, key_fec_params);
    default_rtp_rtcp_->BitrateSent(NULL, sent_video_rate_bps, sent_fec_rate_bps,
        sent_nack_rate_bps);
    return 0;
}

}

