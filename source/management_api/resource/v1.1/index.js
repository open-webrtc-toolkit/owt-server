// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * Router for v1.1 interfaces
 */
'use strict';
const express = require('express')
const router = express.Router()
const streamsResource = require('./streamsResource');
const tokensResource = require('../v1/tokensResource');
const publicationsResource = require('./publicationsResource');
const subscriptionsResource = require('./subscriptionsResource');
const processorsResource = require('./processorsResource');
const participantsResource = require('./participantsResource');
const routerV1 = require('../v1');

// Stream(including external streaming-in) management
router.get('/rooms/:room/streams', streamsResource.getList);
router.get('/rooms/:room/streams/:stream', streamsResource.get);
router.patch('/rooms/:room/streams/:stream', streamsResource.patch);
router.delete('/rooms/:room/streams/:stream', streamsResource.delete);
router.post('/rooms/:room/streaming-ins', streamsResource.addStreamingIn);//FIXME: Validation on body.type === 'streaming' is needed.
router.delete('/rooms/:room/streaming-ins/:stream', streamsResource.delete);

// stream-engine
router.post('/stream-engine/tokens', tokensResource.create);

router.get('/stream-engine/publications', publicationsResource.getList);
router.get('/stream-engine/publications/:publication', publicationsResource.get);
router.post('/stream-engine/publications', publicationsResource.add);
router.patch('/stream-engine/publications/:publication', publicationsResource.patch);
router.delete('/stream-engine/publications/:publication', publicationsResource.delete);

router.get('/stream-engine/subscriptions', subscriptionsResource.getList);
router.get('/stream-engine/subscriptions/:subscription', subscriptionsResource.get);
router.post('/stream-engine/subscriptions', subscriptionsResource.add);
router.patch('/stream-engine/subscriptions/:subscription', subscriptionsResource.patch);
router.delete('/stream-engine/subscriptions/:subscription', subscriptionsResource.delete);

router.get('/stream-engine/nodes', publicationsResource.getNodes);

router.get('/stream-engine/processors', processorsResource.getList);
router.get('/stream-engine/processors/:processors', processorsResource.get);
router.post('/stream-engine/processors', processorsResource.add);
router.delete('/stream-engine/processors/:processor', processorsResource.delete);

router.get('/stream-engine/participants', participantsResource.getList);
router.get('/stream-engine/participants/:participant', participantsResource.get);
router.post('/stream-engine/participants', participantsResource.add);
router.delete('/stream-engine/participants/:participant', participantsResource.delete);

// Same as previous version
router.use(routerV1);

module.exports = router;
