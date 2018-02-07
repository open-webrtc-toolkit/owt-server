#include "WoogeenHandler.h"
#include "WebRtcConnection.h"
#include <rtputils.h>

namespace erizo {

DEFINE_LOGGER(WoogeenHandler, "WoogeenHandler");


void WoogeenHandler::read(Context *ctx, std::shared_ptr<DataPacket> packet) {
  char* buf = packet->data;
  RtcpHeader* chead = reinterpret_cast<RtcpHeader*>(buf);
  RtpHeader* h = reinterpret_cast<RtpHeader*>(buf);
  if (!chead->isRtcp()) {
    int externalPT = h->getPayloadType();
    int internalPT = externalPT;
    if (packet->type == AUDIO_PACKET) {
      internalPT = connection_->getRemoteSdpInfo().getAudioInternalPT(externalPT);
    } else if (packet->type == VIDEO_PACKET) {
      internalPT = connection_->getRemoteSdpInfo().getVideoInternalPT(externalPT);
    }

    if (internalPT == RED_90000_PT) {
      assert(packet->type == VIDEO_PACKET);
      redheader* redhead = (redheader*)(buf + h->getHeaderLength());
      redhead->payloadtype = connection_->getRemoteSdpInfo().getVideoInternalPT(redhead->payloadtype);
    }
  }

  ctx->fireRead(std::move(packet));
}

void WoogeenHandler::write(Context *ctx, std::shared_ptr<DataPacket> packet) {
  char* buf = packet->data;
  int len = packet->length;
  RtpHeader* h = reinterpret_cast<RtpHeader*>(buf);
  RtcpHeader *chead = reinterpret_cast<RtcpHeader*>(buf);
  if (!chead->isRtcp()) {
    // Write SSRC
    if (packet->type == VIDEO_PACKET) {
      h->setSSRC(connection_->getVideoSinkSSRC());
    } else {
      h->setSSRC(connection_->getAudioSinkSSRC());
    }

    int externalRED = connection_->getRemoteSdpInfo().getVideoExternalPT(RED_90000_PT);

    if (h->getPayloadType() == externalRED) {
      int totalLength = h->getHeaderLength();
      int rtpHeaderLength = totalLength;
      redheader *redhead = (redheader*) (buf + totalLength);
      redhead->payloadtype = connection_->getRemoteSdpInfo().getVideoExternalPT(redhead->payloadtype);

      if (!connection_->getRemoteSdpInfo().supportPayloadType(RED_90000_PT)) {
        while (redhead->follow) {
          totalLength += redhead->getLength() + 4; // RED header
          redhead = (redheader*) (buf + totalLength);
        }
        // Parse RED packet to external[payloadType] packet.
        // Copy RTP header
        int newLen = len - 1 - totalLength + rtpHeaderLength;
        assert(newLen <= 3000);

        memcpy(deliverMediaBuffer, buf, rtpHeaderLength);
        // Copy payload data
        memcpy(deliverMediaBuffer + totalLength, buf + totalLength + 1, newLen - rtpHeaderLength);
        // Copy payload type
        RTPHeader* mediahead = (RTPHeader*) deliverMediaBuffer;
        mediahead->setPayloadType(redhead->payloadtype);

        ctx->fireWrite(std::make_shared<DataPacket>(0, deliverMediaBuffer, newLen, VIDEO_PACKET));
        return;
      }
    }
  }

  ctx->fireWrite(std::move(packet));
}

}  // namespace erizo
