// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

class Publication {

  constructor(id, type, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.source = {
      audio: [],
      video: [],
      data: [],
    };
    this.locality = null;
    this.domain = '';
  }

  get status() {
    return this.locality ? 'completed' : 'initializing';
  }

  addSource(trackType, trackId, format, parameters) {
    if (this.source[trackType]) {
      this.source[trackType].push({id: trackId, format, parameters});
      return true;
    } else {
      return false;
    }
  }

  removeSource(trackType, trackId) {
    if (this.source[trackType]) {
      const idx = this.source[trackType]
        .findIndex((track) => (track.id === trackId));
      if (idx >= 0) {
        this.source[trackType].splice(idx, 1);
        return true;
      }
    }
    return false;
  };

  toSignalingFormat() {
    const tracks = [];
    const optional = {
      format: [],
      parameters: {
        resolution: [],
        bitrate: [],
        framerate: [],
        keyFrameInterval: [],
      }
    };
    this.source.audio.forEach((track) => {
        tracks.push(Object.assign({type: 'audio', optional}, track));
    });
    this.source.video.forEach((track) => {
        tracks.push(Object.assign({type: 'video', optional}, track));
    });
    return {
      id: this.id,
      type: "forward",
      media: {
        tracks,
      },
      data: false,
      info: {
        owner: 'admin',
        type: this.type,
        inViews: [],
        attributes: null,
      }
    };
  }
}

class Subscription {
  constructor(id, type, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.sink = {
      audio: [],
      video: [],
      data: [],
    };
    this.locality = null;
    this.domain = '';
  }

  removeSource(trackType, trackFrom) {
    if (this.sink[trackType]) {
      const len = this.sink[trackType].length;
      this.sink[trackType] = this.sink[trackType].filter((track) => {
        return (track.from !== trackFrom);
      });
      return (this.sink[trackType].length !== len);
    }
    return false;
  };

  get status() {
    return this.locality ? 'completed' : 'initializing';
  }
}

class Processor {
  constructor(id, type, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.inputs = {
      audio: [],
      video: [],
      data: [],
    };
    this.outputs = {
      audio: [],
      video: [],
      data: [],
    };
    this.locality = null;
    this.domain = '';
  }
}

exports.Publication = Publication;
exports.Subscription = Subscription;
exports.Processor = Processor;
