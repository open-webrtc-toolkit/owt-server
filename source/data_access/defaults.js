// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const DEFAULT_AUDIO_IN = [
  { codec: 'opus', sampleRate: 48000, channelNum: 2 },
  { codec: 'isac', sampleRate: 16000 },
  { codec: 'isac', sampleRate: 32000 },
  { codec: 'g722', sampleRate: 16000, channelNum: 1 },
  // { codec: 'g722', sampleRate: 16000, channelNum: 2 },
  { codec: 'pcma' },
  { codec: 'pcmu' },
  { codec: 'aac' },
  { codec: 'ac3' },
  { codec: 'nellymoser' },
  { codec: 'ilbc' },
];
const DEFAULT_AUDIO_OUT = [
  { codec: 'opus', sampleRate: 48000, channelNum: 2 },
  { codec: 'isac', sampleRate: 16000 },
  { codec: 'isac', sampleRate: 32000 },
  { codec: 'g722', sampleRate: 16000, channelNum: 1 },
  // { codec: 'g722', sampleRate: 16000, channelNum: 2 },
  { codec: 'pcma' },
  { codec: 'pcmu' },
  { codec: 'aac', sampleRate: 48000, channelNum: 2 },
  { codec: 'ac3' },
  { codec: 'nellymoser' },
  { codec: 'ilbc' },
];
const DEFAULT_VIDEO_IN = [
  { codec: 'h264' },
  { codec: 'vp8' },
  { codec: 'vp9' },
];
const DEFAULT_VIDEO_OUT = [
  { codec: 'vp8' },
  { codec: 'h264', profile: 'CB' },
  { codec: 'vp9' },
];
const DEFAULT_VIDEO_PARA = {
  resolution: ['x3/4', 'x2/3', 'x1/2', 'x1/3', 'x1/4', 'hd1080p', 'hd720p', 'svga', 'vga', 'qvga', 'cif'],
  framerate: [6, 12, 15, 24, 30, 48, 60],
  bitrate: ['x0.8', 'x0.6', 'x0.4', 'x0.2'],
  keyFrameInterval: [100, 30, 5, 2, 1]
};
const DEFAULT_ROLES = [
  {
    role: 'presenter',
    publish: { audio: true, video: true },
    subscribe: { audio: true, video: true }
  },
  {
    role: 'viewer',
    publish: {audio: false, video: false },
    subscribe: {audio: true, video: true }
  },
  {
    role: 'audio_only_presenter',
    publish: {audio: true, video: false },
    subscribe: {audio: true, video: false }
  },
  {
    role: 'video_only_viewer',
    publish: {audio: false, video: false },
    subscribe: {audio: false, video: true }
  },
  {
    role: 'sip',
    publish: { audio: true, video: true },
    subscribe: { audio: true, video: true }
  }
];

const DEFAULT_ROOM_CONFIG = {
  roles: DEFAULT_ROLES,
  mediaIn: {audio: DEFAULT_AUDIO_IN, video: DEFAULT_VIDEO_IN},
  mediaOut: {
    audio: DEFAULT_AUDIO_OUT,
    video: {
      format: DEFAULT_VIDEO_OUT,
      parameters: DEFAULT_VIDEO_PARA
    }
  },
  views: [{}]
};

module.exports = {
  AUDIO_IN: DEFAULT_AUDIO_IN,
  AUDIO_OUT: DEFAULT_AUDIO_OUT,
  VIDEO_IN: DEFAULT_VIDEO_IN,
  VIDEO_OUT: DEFAULT_VIDEO_OUT,
  ROOM_CONFIG: DEFAULT_ROOM_CONFIG
};
