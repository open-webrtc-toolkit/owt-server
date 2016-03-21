# <COPYRIGHT_TAG>


INCLUDES_PATH = -I../../imports/speex/speex/include/ \
                -I../../imports/ffmpeg/include/

ifeq ($(ADEC_ENABLE),1)
INCLUDES_PATH += -I../../imports/ipp/sources/audio-video-codecs/codec/audio/aac/dec/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/codec/audio/aac/enc/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/codec/audio/mp3/enc/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/core/umc/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/core/vm/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/core/vm_plus/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/io/umc/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/codec/audio/aac/common/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/codec/common/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/codec/audio/mp3/dec/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/codec/audio/mp3/common/include/ \
                 -I../../imports/ipp/sources/audio-video-codecs/io/media_buffers/include/ \
                 -I/opt/intel/ipp/include/
endif


ifeq ($(SMTA_ENABLE),1)
LIBS = -lxcodebase -L../../dist -Wl,-rpath='$$ORIGIN/'
EXTRA_FLAGS+=-DSUPPORT_SMTA
EXCLUDE_SRC=app.cpp audio_pcm_reader.cpp audio_pcm_writer.cpp
else
LIBS = -lxcodebase -lspeexdsp -L../../dist -Wl,-rpath='$$ORIGIN/'
endif

ifeq ($(ADEC_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_AUDIO_CODEC
LIBS += -L../../imports/ipp/dist -lmp3_enc -lmp3_dec -lmp3_common \
		-laac_enc -laac_dec -laac_common -lcodec_common \
		-lmedia_buffers -lcore_umc -lvm_plus -lvm   \
		-L/opt/intel/composerxe/ipp/lib/intel64 \
		-Wl,-rpath=/opt/intel/composerxe/ipp/lib/intel64 \
		-lipps -lippcore -lippi -lippac -lippdc \
		-L/opt/intel/composerxe/lib/intel64/ \
		-Wl,-rpath=/opt/intel/composerxe/lib/intel64/ \
		-liomp5
endif

include ../common.mk

