// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_RAW_QUIC_RAW_STREAM_H_
#define NET_TOOLS_QUIC_RAW_QUIC_RAW_STREAM_H_

#include "base/macros.h"
#include "net/third_party/quic/core/quic_stream.h"
#include "net/third_party/quic/core/quic_session.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicRawStream : public QuicStream {
 public:
  // Visitor receives callbacks from the stream.
  class QUIC_EXPORT_PRIVATE Visitor {
   public:
    Visitor() {}
    Visitor(const Visitor&) = delete;
    Visitor& operator=(const Visitor&) = delete;

    // Called when the stream is closed.
    virtual void OnClose(QuicRawStream* stream) = 0;
    virtual void OnData(QuicRawStream* stream, char* data, size_t len) = 0;

   protected:
    virtual ~Visitor() {}
  };

  QuicRawStream(QuicStreamId id,
                QuicSession* session,
                StreamType type);
  QuicRawStream(const QuicRawStream&) = delete;
  QuicRawStream& operator=(const QuicRawStream&) = delete;
  ~QuicRawStream() override;

  // QuicStream implementation called by the sequencer when there is
  // data (or a FIN) to be read.
  void OnDataAvailable() override;

  void set_visitor(Visitor* visitor) { visitor_ = visitor; }

  // Returns true if the sequencer has delivered the FIN, and no more body bytes
  // will be available.
  bool IsClosed() { return sequencer()->IsClosed(); }

 protected:
  Visitor* visitor() { return visitor_; }

 private:
  Visitor* visitor_;
};

}  // namespace quic

#endif  // NET_TOOLS_QUIC_RAW_QUIC_RAW_STREAM_H_
