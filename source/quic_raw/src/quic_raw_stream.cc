
#include "net/tools/quic/raw/quic_raw_stream.h"

#include <list>
#include <utility>

#include "net/third_party/quic/core/quic_utils.h"
#include "net/third_party/quic/platform/api/quic_bug_tracker.h"
#include "net/third_party/quic/platform/api/quic_flags.h"
#include "net/third_party/quic/platform/api/quic_logging.h"
#include "net/third_party/quic/platform/api/quic_map_util.h"
#include "net/third_party/quic/platform/api/quic_text_utils.h"

namespace quic {

QuicRawStream::QuicRawStream(
    QuicStreamId id,
    QuicSession* session,
    StreamType type)
    : QuicStream(id, session, /*is_static=*/false, type),
      visitor_(nullptr) {

}

QuicRawStream::~QuicRawStream() {}

void QuicRawStream::OnDataAvailable() {
  while (sequencer()->HasBytesToRead()) {
    struct iovec iov;
    if (sequencer()->GetReadableRegions(&iov, 1) == 0) {
      // No more data to read.
      break;
    }
    QUIC_DVLOG(1) << "Stream " << id() << " processed " << iov.iov_len
                  << " bytes.";
    if (visitor()) {
      visitor()->OnData(this, static_cast<char*>(iov.iov_base), iov.iov_len);
    }
    sequencer()->MarkConsumed(iov.iov_len);
  }
  if (!sequencer()->IsClosed()) {
    sequencer()->SetUnblocked();
    return;
  }

  // If the sequencer is closed, then all the body, including the fin, has been
  // consumed.
  OnFinRead();

  if (write_side_closed() || fin_buffered()) {
    return;
  }
}

}  // namespace quic
