// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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

#include "AcmmBroadcastGroup.h"
#include "AcmmGroup.h"
#include "AcmmInput.h"

namespace mcu {

//WebRTC Audio Conference Mixer Module Audio Frame Mixer
class AcmmFrameMixer : public AudioFrameMixer,
                       public AudioMixerOutputReceiver,
                       public AudioMixerVadReceiver {
    DECLARE_LOGGER();

    static const int32_t MAX_GROUPS = 10240;
    static const int32_t MIXER_INTERVAL_MS = 10;

    struct OutputInfo {
        owt_base::FrameFormat format;
        owt_base::FrameDestination *dest;
    };

public:
    AcmmFrameMixer();
    virtual ~AcmmFrameMixer();

    // Implements AudioFrameMixer
    void enableVAD(uint32_t period) override;
    void disableVAD() override;
    void resetVAD() override;

    bool addInput(const std::string& group, const std::string& inStream, const owt_base::FrameFormat format, owt_base::FrameSource* source) override;
    void removeInput(const std::string& group, const std::string& inStream) override;

    void setInputActive(const std::string& group, const std::string& inStream, bool active) override;

    bool addOutput(const std::string& group, const std::string& outStream, const owt_base::FrameFormat format, owt_base::FrameDestination* destination) override;
    void removeOutput(const std::string& group, const std::string& outStream) override;

    void setEventRegistry(EventRegistry* handle) override;

    // Implements AudioMixerOutputReceiver
    virtual void NewMixedAudio(
            int32_t id,
            const AudioFrame& generalAudioFrame,
            const AudioFrame** uniqueAudioFrames,
            uint32_t size) override;

    // Implements AudioMixerVadReceiver
    virtual void VadParticipants(
            const ParticipantVadStatistics *statistics,
            const uint32_t size) override;

protected:
    void performMix();

    bool getFreeGroupId(uint16_t *id);

    boost::shared_ptr<AcmmGroup> addGroup(const std::string& group);
    void removeGroup(const std::string& group);
    boost::shared_ptr<AcmmGroup> getGroup(const std::string& group);

    void updateFrequency();

    boost::shared_ptr<AcmmInput> getInputById(int32_t id);

    void statistics();

private:
    EventRegistry *m_asyncHandle;
    boost::scoped_ptr<JobTimer> m_jobTimer;
    boost::shared_ptr<AudioConferenceMixer> m_mixerModule;

    std::map<AcmmOutput*, OutputInfo> m_outputInfoMap;
    boost::shared_ptr<AcmmBroadcastGroup> m_broadcastGroup;

    std::vector<bool> m_groupIds;
    std::map<std::string, uint16_t> m_groupIdMap;
    std::map<uint16_t, boost::shared_ptr<AcmmGroup>> m_groups;
    boost::shared_mutex m_mutex;

    bool m_vadEnabled;
    boost::shared_ptr<AcmmInput> m_mostActiveInput;
    int32_t m_frequency;

    bool m_running;
    boost::thread m_thread;
};

} /* namespace mcu */

#endif /* AcmmFrameMixer_h */
