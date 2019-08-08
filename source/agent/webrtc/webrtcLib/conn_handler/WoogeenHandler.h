// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef ERIZO_EXTRA_WOOGEENHANDLER_H_
#define ERIZO_EXTRA_WOOGEENHANDLER_H_

#include "./logger.h"
#include "pipeline/Handler.h"
#include "MediaStream.h"

namespace erizo {

class MediaStream;

class WoogeenHandler: public Handler, public std::enable_shared_from_this<WoogeenHandler> {
  DECLARE_LOGGER();

 public:
  explicit WoogeenHandler(MediaStream *connection) : connection_{connection}, enabled_{true} {}

  // Enabled always
  void enable() override {}
  void disable() override {}

  std::string getName() override {
     return "woogeen";
  }

  void read(Context *ctx, std::shared_ptr<DataPacket> packet) override;
  void write(Context *ctx, std::shared_ptr<DataPacket> packet) override;
  void notifyUpdate() override {};

 private:
  MediaStream *connection_;
  bool enabled_;
  char deliverMediaBuffer[3000];
};

}  // namespace erizo

#endif  // ERIZO_EXTRA_WOOGEENHANDLER_H_
