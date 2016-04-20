#ifndef OOVOO_PROTOCOL_HEADER_H_
#define OOVOO_PROTOCOL_HEADER_H_

#include <stdint.h>
#include <string>

class OoVooProtocolEventsListener;
class JobTimer;

enum class OovooProtocolErrType
{
	all_user_left = 8,
	invalid_login_info = 9,
	message_err = 10,
	avs_connecton_fail = 11,
	job_timer_fail = 12
};

enum VideoResolutionType
{
	unknown,
	cif,//352x288
	vga,
	hd_720p,
	sif, //320x240
	hvga, //480x320
	r480x360,
	qcif, //176x144
	r192x144
};

enum class VideoCoderType
{
	unknown,
	h264,
	vp8
};

enum class AudioBitRateType
{
	unknown
};

enum class AudioCoderType
{
	unknown,
	opus,
	g711
};

typedef struct
{
	void* pData;
	uint32_t length;
	uint64_t ts;
	uint32_t streamId;
	uint32_t packetId;
	uint32_t frameId;
	uint32_t fragmentId;
	VideoResolutionType resolution;
	bool  isLastFragment;
	bool  isKey;
	bool  isAudio;
	bool  isMirrored;
	uint32_t  rotation;
} mediaPacket_t;

class JobTimer
{
public:
	virtual void setStopped() = 0;
	virtual void setRunning() = 0;
	virtual bool isRunning() = 0;
	virtual void setTimer(uint64_t frequency) = 0;
};

class OoVooProtocolStack
{
public:
	OoVooProtocolStack(OoVooProtocolEventsListener* listener, std::string codecCapablity)
		: _eventsListener(listener) {
	}
	virtual ~OoVooProtocolStack() { }

	virtual void clientJoin(std::string uri) = 0; //Called when there is a webrtc client ask to join, uri is get from web server used to identify the user and the group that it will join
	virtual void clientLeave() = 0;//Called when webRTC client is leaving

	//Stream from webrtc to ooVoo
	virtual void outboundStreamCreate(uint32_t userId, bool isAudio, uint16_t resolution) = 0;//Called when webrtc media channel is setup for video/audio to send out
	virtual void outboundStreamClose(uint32_t streamId) = 0;//video/audio sending is paused by webrtc user
	virtual void dataDeliver(mediaPacket_t* packet) = 0;//when media data available from webrtc
	virtual void customMessage(void* pData, uint32_t length) = 0;

	//Stream from ooVoo to webrtc
	virtual void inboundClientCreated(int errorCode) = 0; //When p2p connection established successfully for the inbound client
	virtual void inboundStreamCreate(uint32_t userId, bool isAudio) = 0;//Called when webrtc media channel is setup for video/audio to receive from ooVoo
	virtual void inboundStreamClose(uint32_t streamId) = 0;//video/audio receiving is paused by webrtc user

	//GW will take care of tcp/udp port setup
	virtual void avsConnectionSuccess(bool isUdp) = 0;//Called when port is setup to connect AVS
	virtual void avsConnectionFail(bool isUdp) = 0;//Called when port is failed to setup
	virtual size_t dataAvailable(void *pMessage, uint32_t length, bool isUdp = false) = 0;//Called when receive data from AVS

	virtual void syncMsg(uint64_t ntp, uint64_t rtp, bool isAudio) = 0;
	virtual void inboundPacketLoss(double percentage) = 0;

	//	//GW need to maintain a timer for ooVoo
	virtual void JobTimerCreated(JobTimer* pTimer) = 0;
	virtual void JobTimerCreateFailed() = 0;
	virtual void JobTimerPulse() = 0;

	static OoVooProtocolStack* CreateOoVooProtocolStack(OoVooProtocolEventsListener* listener, std::string codecCapablity);

protected:
	OoVooProtocolEventsListener*				_eventsListener;
};

class OoVooProtocolEventsListener {
public:

	virtual void onClientJoin(uint32_t userId, VideoCoderType vType, AudioCoderType aType, int errorCode) = 0;//Notify GW whether the client join the conference successfully
	virtual void onClientLeave(int errorCode) = 0;//Called when client is disconnect from AVS

	//Stream from webrtc to ooVoo
	virtual void onOutboundStreamCreate(uint32_t streamId, bool isAudio, int errorCode) = 0;//Notify whether the stream is created successfully
	virtual void onOutboundStreamClose(uint32_t streamId, int errorCode) = 0;//Notify whether the stream is closed
	virtual void onRequestIFrame() = 0;

	//Stream from ooVoo to webrtc
	virtual void onCreateInboundClient(std::string userName, uint32_t userId, std::string participantInfo) = 0;//New client join group
	virtual void onCloseInboundClient(uint32_t userId) = 0;//Client leave group
	virtual void onInboundStreamCreate(uint32_t userId, uint32_t streamId, bool isAudio, uint16_t resolution) = 0;//Notify webrtc whether video/audio is ready to be sent
	virtual void onInboundStreamClose(uint32_t streamId) = 0;// Notify when ooVoo client's video/audio is off
	virtual void onDataReceive(mediaPacket_t* packet) = 0;//Called when there is a media packet received from ooVoo client
	virtual void onCustomMessage(void* pData, uint32_t length) = 0;

	//GW will take care of tcp/udp port setup
	virtual void onEstablishAVSConnection(std::string ip, uint32_t port, bool isUdp) = 0; //Ask GW to establish a connection to AVS
	virtual void onSentMessage(void *pMessage, uint32_t length, bool isUdp) = 0; //Send message to AVS

	virtual void onSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId) = 0;

};
#endif // OOVOO_PROTOCOL_HEADER_H_
