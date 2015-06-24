/**
 * VideoCodec.pp
 */

#include "VideoCodec.h"

#include <cstdio>
#include <string.h>

namespace erizo {

  DEFINE_LOGGER(VideoEncoder, "media.codecs.VideoEncoder");
  DEFINE_LOGGER(VideoDecoder, "media.codecs.VideoDecoder");

  inline  AVCodecID
    VideoCodecID2ffmpegDecoderID(VideoCodecID codec)
    {
      switch (codec)
      {
        case VIDEO_CODEC_H264: return AV_CODEC_ID_H264;
        case VIDEO_CODEC_VP8: return AV_CODEC_ID_VP8;
        case VIDEO_CODEC_MPEG4: return AV_CODEC_ID_MPEG4;
        default: return AV_CODEC_ID_VP8;
      }
    }

  VideoEncoder::VideoEncoder(){
    avcodec_register_all();
    vCoder = NULL;
    vCoderContext = NULL;
    cPicture = NULL;
  }

  VideoEncoder::~VideoEncoder(){
    this->closeEncoder();
  }

  int VideoEncoder::initEncoder(const VideoCodecInfo& info){
    vCoder = avcodec_find_encoder(VideoCodecID2ffmpegDecoderID(info.codec));
    if (!vCoder) {
      ELOG_DEBUG("Video codec not found for encoder");
      return -1;
    }

    vCoderContext = avcodec_alloc_context3(vCoder);
    if (!vCoderContext) {
      ELOG_DEBUG("Error allocating vCoderContext");
      return -2;
    }

    vCoderContext->bit_rate = info.bitRate;
    vCoderContext->rc_min_rate = info.bitRate; //
    vCoderContext->rc_max_rate = info.bitRate; // VPX_CBR
    vCoderContext->qmin = 0;
    vCoderContext->qmax = 40; // rc_quantifiers
    vCoderContext->profile = 3;
    //    vCoderContext->frame_skip_threshold = 30;
    vCoderContext->rc_buffer_aggressivity = 0.95;
    //vCoderContext->rc_buffer_size = vCoderContext->bit_rate;
    //vCoderContext->rc_initial_buffer_occupancy = vCoderContext->bit_rate / 2;
    vCoderContext->rc_initial_buffer_occupancy = 500;

    vCoderContext->rc_buffer_size = 1000;

    vCoderContext->width = info.width;
    vCoderContext->height = info.height;
    vCoderContext->pix_fmt = PIX_FMT_YUV420P;
    vCoderContext->time_base = (AVRational) {1, 90000};
    //
    vCoderContext->sample_aspect_ratio =
      (AVRational) {info.width,info.height};
    vCoderContext->thread_count = 4;

    if (avcodec_open2(vCoderContext, vCoder, NULL) < 0) {
      ELOG_DEBUG("Error opening video decoder");
      return -3;
    }

    cPicture = av_frame_alloc();
    if (!cPicture) {
      ELOG_DEBUG("Error allocating video frame");
      return -4;
    }

    ELOG_DEBUG("videoCoder configured successfully %d x %d", vCoderContext->width,
        vCoderContext->height);
    return 0;
  }

  int VideoEncoder::encodeVideo (unsigned char* inBuffer, int inLength, unsigned char* outBuffer, int outLength, int& hasFrame){

    int size = vCoderContext->width * vCoderContext->height;
    //    ELOG_DEBUG("vCoderContext width %d", vCoderContext->width);

    cPicture->pts = AV_NOPTS_VALUE;
    cPicture->data[0] = inBuffer;
    cPicture->data[1] = inBuffer + size;
    cPicture->data[2] = inBuffer + size + size / 4;
    cPicture->linesize[0] = vCoderContext->width;
    cPicture->linesize[1] = vCoderContext->width / 2;
    cPicture->linesize[2] = vCoderContext->width / 2;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = outBuffer;
    pkt.size = outLength;

    int ret = 0;
    int got_packet = 0;
    //    ELOG_DEBUG(
    //        "Before encoding inBufflen %d, size %d, codecontext width %d pkt->size%d",
    //        inLength, size, vCoderContext->width, pkt.size);
    ret = avcodec_encode_video2(vCoderContext, &pkt, cPicture, &got_packet);
    //    ELOG_DEBUG("Encoded video size %u, ret %d, got_packet %d, pts %lld, dts %lld",
    //        pkt.size, ret, got_packet, pkt.pts, pkt.dts);
    if (!ret && got_packet && vCoderContext->coded_frame) {
      vCoderContext->coded_frame->pts = pkt.pts;
      vCoderContext->coded_frame->key_frame =
        !!(pkt.flags & AV_PKT_FLAG_KEY);
    }
    return ret ? ret : pkt.size;
  }

  int VideoEncoder::closeEncoder() {
    if (vCoderContext!=NULL)
      avcodec_close(vCoderContext);
    if (cPicture !=NULL)
      av_frame_free(&cPicture);

    return 0;
  }


  VideoDecoder::VideoDecoder(){
    avcodec_register_all();
    vDecoder = NULL;
    vDecoderContext = NULL;
    dPicture = NULL;
    initWithContext_=false;
  }

  VideoDecoder::~VideoDecoder(){
    this->closeDecoder();
  }

  int VideoDecoder::initDecoder (const VideoCodecInfo& info){
    ELOG_DEBUG("Init Decoder");
    vDecoder = avcodec_find_decoder(VideoCodecID2ffmpegDecoderID(info.codec));
    if (!vDecoder) {
      ELOG_DEBUG("Error getting video decoder");
      return -1;
    }

    vDecoderContext = avcodec_alloc_context3(vDecoder);
    if (!vDecoderContext) {
      ELOG_DEBUG("Error getting allocating decoder context");
      return -1;
    }

    vDecoderContext->width = info.width;
    vDecoderContext->height = info.height;

    if (avcodec_open2(vDecoderContext, vDecoder, NULL) < 0) {
      ELOG_DEBUG("Error opening video decoder");
      return -1;
    }

    dPicture = av_frame_alloc();
    if (!dPicture) {
      ELOG_DEBUG("Error allocating video frame");
      return -1;
    }

    return 0;
  }

  int VideoDecoder::initDecoder (AVCodecContext* context){    
    ELOG_DEBUG("Init Decoder with context");
    initWithContext_=true;
    vDecoder = avcodec_find_decoder(context->codec_id);
    if (!vDecoder) {
      ELOG_DEBUG("Error getting video decoder");
      return -1;
    }
    vDecoderContext = context;

    if (avcodec_open2(vDecoderContext, vDecoder, NULL) < 0) {
      ELOG_DEBUG("Error opening video decoder");
      return -1;
    }

    dPicture = av_frame_alloc();
    if (!dPicture) {
      ELOG_DEBUG("Error allocating video frame");
      return -1;
    }

    return 0;
  }

  int VideoDecoder::decodeVideo(unsigned char* inBuff, int inBuffLen,
      unsigned char* outBuff, int outBuffLen, int* gotFrame){
    if (vDecoder == 0 || vDecoderContext == 0){
      ELOG_DEBUG("Init Codec First");
      return -1;
    }

    *gotFrame = false;

    AVPacket avpkt;
    av_init_packet(&avpkt);

    avpkt.data = inBuff;
    avpkt.size = inBuffLen;

    int got_picture;
    int len;

    while (avpkt.size > 0) {

      len = avcodec_decode_video2(vDecoderContext, dPicture, &got_picture,
          &avpkt);

      if (len < 0) {
        ELOG_DEBUG("Error decoding video frame");
        return -1;
      }

      if (got_picture) {
        *gotFrame = 1;
        goto decoding;
      }
      avpkt.size -= len;
      avpkt.data += len;
    }

    if (!got_picture) {
      return -1;
    }

decoding:

    int outSize = vDecoderContext->height * vDecoderContext->width;

    if (outBuffLen < (outSize * 3 / 2)) {
      return outSize * 3 / 2;
    }

    unsigned char *lum = outBuff;
    unsigned char *cromU = outBuff + outSize;
    unsigned char *cromV = outBuff + outSize + outSize / 4;

    unsigned char *src = NULL;
    int src_linesize, dst_linesize;

    src_linesize = dPicture->linesize[0];
    dst_linesize = vDecoderContext->width;
    src = dPicture->data[0];

    for (int i = vDecoderContext->height; i > 0; i--) {
      memcpy(lum, src, dst_linesize);
      lum += dst_linesize;
      src += src_linesize;
    }

    src_linesize = dPicture->linesize[1];
    dst_linesize = vDecoderContext->width / 2;
    src = dPicture->data[1];

    for (int i = vDecoderContext->height / 2; i > 0; i--) {
      memcpy(cromU, src, dst_linesize);
      cromU += dst_linesize;
      src += src_linesize;
    }

    src_linesize = dPicture->linesize[2];
    dst_linesize = vDecoderContext->width / 2;
    src = dPicture->data[2];

    for (int i = vDecoderContext->height / 2; i > 0; i--) {
      memcpy(cromV, src, dst_linesize);
      cromV += dst_linesize;
      src += src_linesize;
    }
    av_free_packet(&avpkt);

    return outSize * 3 / 2;
  }

  int VideoDecoder::closeDecoder(){
    if (!initWithContext_ && vDecoderContext != NULL)
      avcodec_close(vDecoderContext);
    if (dPicture != NULL)
      av_frame_free(&dPicture);
    return 0;
  }

}
