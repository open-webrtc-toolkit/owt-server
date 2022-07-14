// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

const extMappings = [
  'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
  'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01',
  'urn:ietf:params:rtp-hdrext:sdes:mid',
  'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id',
  'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id',
  'urn:ietf:params:rtp-hdrext:toffset',
  'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
  // 'urn:3gpp:video-orientation',
  // 'http://www.webrtc.org/experiments/rtp-hdrext/playout-delay',
];

const vp8 = {
  payloadType: 100,
  encodingName: 'VP8',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
  feedbackTypes: [
    'ccm fir',
    'nack',
    'transport-cc',
    'goog-remb',
  ],
};

const vp9 = {
  payloadType: 101,
  encodingName: 'VP9',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
  feedbackTypes: [
    'ccm fir',
    'nack',
    'transport-cc',
    'goog-remb',
  ],
};

const h264 = {
  payloadType: 127,
  encodingName: 'H264',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
  feedbackTypes: [
    'ccm fir',
    'nack',
    'transport-cc',
    'goog-remb',
  ],
};

const h265 = {
  payloadType: 121,
  encodingName: 'H265',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
  feedbackTypes: [
    'ccm fir',
    'nack',
    'transport-cc',
    'goog-remb',
  ],
};

const av1 = {
  payloadType: 35,
  encodingName: 'AV1',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
  feedbackTypes: [
    'ccm fir',
    'nack',
    'transport-cc',
    'goog-remb',
  ],
};

const red = {
  payloadType: 116,
  encodingName: 'red',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
};

const rtx = {
  payloadType: 96,
  encodingName: 'rtx',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
};

const ulpfec = {
  payloadType: 117,
  encodingName: 'ulpfec',
  clockRate: 90000,
  channels: 1,
  mediaType: 'video',
};

const opus = {
  payloadType: 120,
  encodingName: 'opus',
  clockRate: 48000,
  channels: 2,
  mediaType: 'audio',
};

const isac16 = {
  payloadType: 103,
  encodingName: 'ISAC',
  clockRate: 16000,
  channels: 1,
  mediaType: 'audio',
};

const isac32 = {
  payloadType: 104,
  encodingName: 'ISAC',
  clockRate: 32000,
  channels: 1,
  mediaType: 'audio',
};

// Since native SdpInfo ignores channels
// g722 matches both g722/1 and g722/2
const g722 = {
  payloadType: 9,
  encodingName: 'G722',
  clockRate: 8000,
  channels: 1,
  mediaType: 'audio',
};

const ilbc = {
  payloadType: 102,
  encodingName: 'ILBC',
  clockRate: 8000,
  channels: 1,
  mediaType: 'audio',
};

const pcmu = {
  payloadType: 0,
  encodingName: 'PCMU',
  clockRate: 8000,
  channels: 1,
  mediaType: 'audio',
};

const pcma = {
  payloadType: 8,
  encodingName: 'PCMA',
  clockRate: 8000,
  channels: 1,
  mediaType: 'audio',
};

const cn8 = {
  payloadType: 13,
  encodingName: 'CN',
  clockRate: 8000,
  channels: 1,
  mediaType: 'audio',
};

const cn16 = {
  payloadType: 105,
  encodingName: 'CN',
  clockRate: 16000,
  channels: 1,
  mediaType: 'audio',
};

const cn32 = {
  payloadType: 106,
  encodingName: 'CN',
  clockRate: 32000,
  channels: 1,
  mediaType: 'audio',
};

const cn48 = {
  payloadType: 107,
  encodingName: 'CN',
  clockRate: 48000,
  channels: 1,
  mediaType: 'audio',
};

const telephoneevent = {
  payloadType: 126,
  encodingName: 'telephone-event',
  clockRate: 8000,
  channels: 1,
  mediaType: 'audio',
};

const mediaConfig = {
  default: {
    rtpMappings: {
      vp8, vp9, h264, h265,av1, red, rtx,
      opus, pcmu, pcma, isac16, isac32, g722, ilbc,
      ulpfec, telephoneevent
    },
    extMappings
  },
  VP8_AND_OPUS: { rtpMappings: { vp8, opus }, extMappings },
  VP9_AND_OPUS: { rtpMappings: { vp9, opus }, extMappings },
  H264_AND_OPUS: { rtpMappings: { h264, opus }, extMappings },
};

module.exports = mediaConfig;
