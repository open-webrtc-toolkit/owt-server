// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
const AudioMixer = require('../audioMixer/build/Release/audioMixer');
const { AudioRanker } = require('../audioRanker/build/Release/audioRanker');

const logger = require('../logger').logger;
const log = logger.getLogger('SelectiveMixer');

const DEFAULT_K = 3;
const DETECT_MUTE = true;
const CHANGE_INTERVAL = 200;

class SelectiveMixer {

    constructor(k, config) {
        k = (k > 0) ? k : DEFAULT_K;
        // streamId : { owner, codec, conn }
        this.inputs = new Map();
        this.mixer = new AudioMixer(config);
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
                    log.debug('Mixer removeInput:', owner, streamId);
                    this.mixer.removeInput(owner, streamId);
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
                log.debug('Mixer addInput:', owner, streamId, codec, i);
                this.mixer.addInput(owner, streamId, codec, this.topK[i].source());
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
            let i = this.currentRank.indexOf(streamId);
            if (i >= 0) {
                this.mixer.removeInput(owner, streamId);
                this.currentRank[i] = '';
            }
        } else {
            log.warn(`Remove non-add input ${streamId}`);
        }
    }

    addOutput(forWhom, streamId, codec, dispatcher) {
        log.debug('addOutput:', forWhom, streamId, codec);
        return this.mixer.addOutput(forWhom, streamId, codec, dispatcher);
    }

    removeOutput(forWhom, streamId) {
        log.debug('removeOutput:', forWhom, streamId);
        this.mixer.removeOutput(forWhom, streamId);
    }

    enableVAD(period, cb) {
        this.mixer.enableVAD(period, cb);
    }

    resetVAD() {
        this.mixer.resetVAD();
    }

    setInputActive(owner, streamId, active) {
        if (this.currentRank.indexOf(streamId) >= 0) {
            this.mixer.setInputActive(owner, streamId, active);
        }
    }

    close() {
        this.mixer.close();
    }
}

exports.SelectiveMixer = SelectiveMixer;
