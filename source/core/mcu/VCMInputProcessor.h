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

#ifndef VCMInputProcessor_h
#define VCMInputProcessor_h

#include "BufferManager.h"
#include "VCMMediaProcessorHelper.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/video_engine/vie_receiver.h>

using namespace webrtc;

namespace mcu {

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.  Each publisher will be
 * served by one VCMInputProcessor.
 * This class more or less is working as the vie_receiver class
 */
class ACMInputProcessor;
class AVSyncModule;
class TaskRunner;

class VCMInputProcessor : public erizo::RTPDataReceiver,
                          public VCMReceiveCallback {
    DECLARE_LOGGER();

public:
    VCMInputProcessor(int index);
    virtual ~VCMInputProcessor();

    // Implements the webrtc::VCMReceiveCallback interface.
    virtual int32_t FrameToRender(webrtc::I420VideoFrame&);

    // Implements the erizo::RTPDataReceiver interface.
    virtual void receiveRtpData(char* rtpdata, int len, erizo::DataType type = erizo::VIDEO, uint32_t streamId = 0);

    bool init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<BufferManager>, boost::shared_ptr<InputFrameCallback>, boost::shared_ptr<TaskRunner>);
    void close();

    void bindAudioInputProcessor(boost::shared_ptr<ACMInputProcessor>);
    int channelId() { return index_;}

private:
    int index_; //the index number of this publisher
    VideoCodingModule* vcm_;
    boost::scoped_ptr<RemoteBitrateObserver> m_remoteBitrateObserver;
    boost::scoped_ptr<RemoteBitrateEstimator> m_remoteBitrateEstimator;
    boost::scoped_ptr<ViEReceiver> m_videoReceiver;
    boost::scoped_ptr<woogeen_base::WoogeenTransport<erizo::VIDEO>> m_videoTransport;
    boost::scoped_ptr<RtpRtcp> rtp_rtcp_;
    boost::scoped_ptr<AVSyncModule> avSync_;

    boost::shared_ptr<ACMInputProcessor> aip_;
    boost::shared_ptr<InputFrameCallback> frameReadyCB_;
    boost::shared_ptr<BufferManager> bufferManager_;
    boost::shared_ptr<TaskRunner> taskRunner_;

    boost::scoped_ptr<DebugRecorder> recorder_;
};

class DummyRemoteBitrateObserver : public RemoteBitrateObserver {
public:
    virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs, unsigned int bitrate) {}
};

}
#endif /* VCMInputProcessor_h */
