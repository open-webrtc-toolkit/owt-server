// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { EventEmitter } = require('events');
const MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
const { AudioRanker } = require('../audioRanker/build/Release/audioRanker');

const logger = require('../logger').logger;
const log = logger.getLogger('ActiveAudioSelector');

const DEFAULT_K = 3;
const DETECT_MUTE = true;
const CHANGE_INTERVAL = 200;

class ActiveAudioSelector extends EventEmitter {

    constructor(k) {
        super();
        k = (k > 0) ? k : DEFAULT_K;
        // streamId : { owner, codec, conn }
        this.inputs = new Map();
        this.ranker = new AudioRanker(
            this._onRankChange.bind(this), DETECT_MUTE, CHANGE_INTERVAL);
        this.topK = [];
        for (let i = 0; i < k; i++) {
            let multicaster = new MediaFrameMulticaster();
            this.topK.push(multicaster);
            this.ranker.addOutput(this.topK[i]);
        }
        this.currentRank = Array(k).fill('');
        log.debug('Init with K:', k);
    }

    _onRankChange(jsonChanges) {
        log.debug('_onRankChange:', jsonChanges);
        let changes = JSON.parse(jsonChanges);
        // Get rank difference
        if (changes.length !== this.currentRank.length) {
            log.warn('Changes number not equal to K');
            return;
        }

        let pendingAddIndexes = [];
        for (let i = 0; i < this.currentRank.length; i++) {
            if (this.currentRank[i] !== changes[i][0]) {
                // Remove previous input
                let streamId = this.currentRank[i];
                if (this.inputs.has(streamId)) {
                    let { owner, codec } = this.inputs.get(streamId);
                    log.debug('Inactive input:', owner, streamId);
                } else if (streamId !== '') {
                    log.warn('Unknown stream ID', streamId);
                }
                pendingAddIndexes.push(i);
            }
        }
        for (let i of pendingAddIndexes) {
            // Re-add top K with updated owner/streamId/codec
            let streamId = changes[i][0];
            if (this.inputs.has(streamId)) {
                let { owner, codec } = this.inputs.get(streamId);
                log.debug('Active input:', owner, streamId, codec, i);
                this.emit('source-change', i, owner, streamId, codec);
            } else if (streamId !== '') {
                log.warn('Unknown stream ID', streamId);
            }
        }
        this.currentRank = changes.map(pair => pair[0]);
    }

    addInput(owner, streamId, codec, conn) {
        log.debug('addInput:', owner, streamId, codec);
        if (!this.inputs.has(streamId)) {
            this.inputs.set(streamId, { owner, codec, conn });
            this.ranker.addInput(conn, streamId, owner);
            return true;
        } else {
            log.warn(`Duplicate addInput ${streamId}`);
            return false;
        }
    }

    removeInput(owner, streamId) {
        log.debug('removeInput:', owner, streamId);
        if (this.inputs.delete(streamId)) {
            this.ranker.removeInput(streamId);
        } else {
            log.warn(`Remove non-add input ${streamId}`);
        }
    }

    removeOutput() {}

    getOutputs() {
        return this.topK;
    }
}

exports.ActiveAudioSelector = ActiveAudioSelector;
