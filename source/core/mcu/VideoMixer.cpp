/*
 * VideoMixer.cpp
 */

#include "VideoMixer.h"

#include "BufferManager.h"
#include "VCMMediaProcessor.h"
#include <ProtectedRTPReceiver.h>
#include <ProtectedRTPSender.h>
#include <WebRtcConnection.h>
#include <WoogeenTransport.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

  DEFINE_LOGGER(VideoMixer, "media.mixers.VideoMixer");

  VideoMixer::VideoMixer() {
     this->setVideoSourceSSRC(55543);
     this->setAudioSourceSSRC(44444);
     this->init();
  }

  VideoMixer::~VideoMixer() {
	  delete bufferManager_;
	  delete transport_;
	  delete subscriber_;
	  delete op_;
  }



  /**
   * init could be used for reset the state of this VideoMixer
   */
  bool VideoMixer::init() {

	  bufferManager_ = new BufferManager();
      subscriber_ = new erizo::OneToManyProcessor();
      subscriber_->setPublisher(this);
      transport_ = new WoogeenTransport(subscriber_);
      op_ = new VCMOutputProcessor();
      op_->init(transport_, bufferManager_, this);

      return true;

  }

  int VideoMixer::deliverAudioData(char* buf, int len, MediaSource* from) {
      //	ELOG_DEBUG("m.videoCodec.bitrate %d\n", m.videoCodec.bitRate);
	  return 0;
  }

  /**
   * use vcm to decode/compose/encode the streams, and then deliver to all subscribers
   * multiple publishers may call to this method simultaneously from different threads.
   * the incoming buffer is a rtp packet
   */
  int VideoMixer::deliverVideoData(char* buf, int len, MediaSource* from) {
	  ProtectedRTPReceiver* receiver = publishers_[from];
	  if (receiver == NULL)
		  return 0;		// Is this possible when remove publisher is called while media stream still comes in?
	  receiver->deliverVideoData(buf, len);
	  return 0;
  }


  /**
   * Attach a new InputStream to the Transcoder
   */
  void VideoMixer::addPublisher(MediaSource* puber){
	  int index = assignSlot(puber);
	  ELOG_DEBUG("addPublisher - assigned slot is %d", index);
	  if (publishers_[puber] == NULL) {
		  op_->updateMaxSlot(maxSlot());
		  boost::shared_ptr<VCMInputProcessor> ip(new VCMInputProcessor(index, op_));
		  ip->init(bufferManager_);
		  ProtectedRTPReceiver* protectedRTPReceiver = new ProtectedRTPReceiver(ip);
		  publishers_[puber] = protectedRTPReceiver;
		 // publishers_.insert(std::pair<MediaSource*, woogeen_base::ProtectedRTPReceiver*>(puber, protectedRTPReceiver));

	  } else {
		  assert (!"new publisher added with InputProcessor still available");	// should not go there
	  }
  }

  /**
   * Attach a new OutputStream to the OneToManyProcessor
   */
  void VideoMixer::addSubscriber(MediaSink* suber, const std::string& peerId){
	  ELOG_DEBUG("Adding subscriber: videoSinkSSRC is %d", suber->getVideoSinkSSRC());
	  subscriber_->addSubscriber(suber, peerId);

  }

  void VideoMixer::removeSubscriber(const std::string& peerId){
	  ELOG_DEBUG("removing subscriber: peerId is %s",   peerId.c_str());
	  subscriber_->removeSubscriber(peerId);

  }

  void VideoMixer::removePublisher(MediaSource* puber) {
	  if (publishers_[puber]) {
		  int index = getSlot(puber);
		  assert(index >= 0);
		  puberSlotMap_[index] = NULL;
		  delete publishers_[puber];
		  publishers_.erase(puber);
	  }

  }

  void VideoMixer::closeSink() {
	  //    this->closeAll();
  }

  void VideoMixer::closeAll() {
  }

}/* namespace mcu */

