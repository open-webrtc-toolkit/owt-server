// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
const { AudioRanker } = require('../audioRanker/build/Release/audioRanker');

const logger = require('../logger').logger;
const log = logger.getLogger('LocalAudioRank');

const DEFAULT_K = 3;
const DETECT_MUTE = false;
const CHANGE_INTERVAL = 200;

const randomId = function(length) {
    return Math.random().toPrecision(length)
        .toString().substr(2, length);
};

// Used to wrap WrtcConn as FrameSource
class FramePipe {
    constructor(wrtcConn) {
        this.wrtcConn = wrtcConn;
        this.multicaster = new MediaFrameMulticaster();
        this.wrtcConn.addDestination('audio', this.multicaster);
    }

    source() {
        return this.multicaster.source();
    }

    destination() {
        return this.multicaster;
    }

    close() {
        this.wrtcConn.removeDestination('audio', this.multicaster);
        this.multicaster.close();
    }
}

class LocalAudioRank {
    constructor(k, audioNode, notifyCb) {
        k = (k > 0) ? k : DEFAULT_K;
        this.audioNode = audioNode;
        this.notifyCallback = notifyCb;
        // streamId - { codec, pipe }
        this.inputs = new Map();
        this.ranker = new AudioRanker(
            this._onRankChange.bind(this), DETECT_MUTE, CHANGE_INTERVAL);
        this.topK = [];
        this.outputIds = [];
        for (let i = 0; i < k; i++) {
            let multicaster = new MediaFrameMulticaster();
            this.topK.push(multicaster);
            this.ranker.addOutput(this.topK[i]);
            this.outputIds.push(randomId(20));
        }
        this.currentRank = Array(k).fill('');
        log.debug('Init with K:', k);
    }

    rank() {
        return this.topK.map((multicaster, i) => ({
            id: this.outputIds[i],
            conn: multicaster
        }));
    }

    _onRankChange(jsonChanges) {
        log.debug('_onRankChange:', jsonChanges);
        let changes = JSON.parse(jsonChanges);
        // Get rank difference
        if (changes.length !== this.currentRank.length) {
            log.info('Changes number not equal to K');
            return;
        }

        // Call RPC to audio node with codec
        let updates = changes.map((change, i) => {
            let info = this.inputs.get(change[0]);
            let codec = info ? info.codec : undefined;
            return {
                id: this.outputIds[i],
                originId: change[0],
                owner: change[1],
                codec: codec
            };
        });

        this.currentRank = changes.map(pair => pair[0]);
        this.notifyCallback(this.audioNode, updates);
    }

    addLocalAudio(streamId, owner, conn, codec) {
        // Save codec
        if (!this.inputs.has(streamId)) {
            let pipe = new FramePipe(conn);
            this.inputs.set(streamId, { codec, pipe });
            this.ranker.addInput(pipe.source(), streamId, owner);
            return true;
        } else {
            log.warn('Add duplicate input:', streamId);
            return false;
        }
    }

    removeLocalAudio(streamId) {
        if (this.inputs.has(streamId)) {
            this.ranker.removeInput(streamId);
            this.inputs.get(streamId).pipe.close();
            this.inputs.delete(streamId);
            return true;
        } else {
            log.debug('Remove non-added input:', streamId);
            return false;
        }
        
    }

}

exports.LocalAudioRank = LocalAudioRank;
