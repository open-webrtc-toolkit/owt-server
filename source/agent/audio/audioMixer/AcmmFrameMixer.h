/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#ifndef AcmmFrameMixer_h
#define AcmmFrameMixer_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <JobTimer.h>
#include <EventRegistry.h>

#include <webrtc/modules/audio_conference_mixer/include/audio_conference_mixer.h>
#include <webrtc/modules/audio_conference_mixer/include/audio_conference_mixer_defines.h>

#include "MediaFramePipeline.h"
#include "AudioFrameMixer.h"
#include "AcmmParticipant.h"

namespace mcu {

//WebRTC Audio Conference Mixer Module Audio Frame Mixer
class AcmmFrameMixer : public AudioFrameMixer,
                       public JobTimerListener,
                       public AudioMixerOutputReceiver,
                       public AudioMixerStatusReceiver {
    DECLARE_LOGGER();

    static const int32_t MAX_PARTICIPANTS = 128;
    static const int32_t MIXER_FREQUENCY = 100;
    static const int32_t MAX_SAMPLE_RATE = 48000;

public:
    AcmmFrameMixer();
    virtual ~AcmmFrameMixer();

    // Implements AudioFrameMixer
    void enableVAD(uint32_t period) override;
    void disableVAD() override;
    void resetVAD() override;

    bool addInput(const std::string& participant, const woogeen_base::FrameFormat format, woogeen_base::FrameSource* source) override;
    void removeInput(const std::string& participant) override;
    bool addOutput(const std::string& participant, const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination) override;
    void removeOutput(const std::string& participant) override;

    void setEventRegistry(EventRegistry* handle) override;

    // Implements JobTimerListener
    void onTimeout() override;

    // Implements AudioMixerOutputReceiver
    virtual void NewMixedAudio(
            int32_t id,
            const AudioFrame& generalAudioFrame,
            const AudioFrame** uniqueAudioFrames,
            uint32_t size) override;

    // Implements AudioMixerStatusReceiver
    virtual void MixedParticipants(
            const int32_t id,
            const ParticipantStatistics* participantStatistics,
            const uint32_t size) override;

    virtual void VADPositiveParticipants(
            const int32_t id,
            const ParticipantStatistics* participantStatistics,
            const uint32_t size) override;

    virtual void MixedAudioLevel(
            const int32_t id,
            const uint32_t level) override;

protected:
    void performMix();

    int32_t getFreeId();

    int32_t addParticipant(const std::string& participant);
    void removeParticipant(const std::string& participant);
    boost::shared_ptr<AcmmParticipant> getParticipant(const std::string& participant);

    void updateFrequency();

private:
    EventRegistry *m_asyncHandle;
    boost::scoped_ptr<JobTimer> m_jobTimer;
    boost::shared_ptr<AudioConferenceMixer> m_mixerModule;

    std::vector<bool> m_freeIds;
    std::map<std::string, int32_t> m_ids;
    std::map<int32_t, boost::shared_ptr<AcmmParticipant>> m_participants;
    uint32_t m_inputs;
    uint32_t m_outputs;
    boost::shared_mutex m_mutex;

    AudioFrame mutedAudioFrame;

    bool m_vadEnabled;
    int32_t m_mostActiveChannel;
    int32_t m_frequency;
};

} /* namespace mcu */

#endif /* AcmmFrameMixer_h */
