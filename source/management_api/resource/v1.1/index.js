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
const routerV1 = require('../v1');

// Stream(including external streaming-in) management
router.get('/rooms/:room/streams', streamsResource.getList);
router.get('/rooms/:room/streams/:stream', streamsResource.get);
router.patch('/rooms/:room/streams/:stream', streamsResource.patch);
router.delete('/rooms/:room/streams/:stream', streamsResource.delete);
router.post('/rooms/:room/streaming-ins', streamsResource.addStreamingIn);//FIXME: Validation on body.type === 'streaming' is needed.
router.delete('/rooms/:room/streaming-ins/:stream', streamsResource.delete);

// Cluster
router.post('/cluster/tokens', tokensResource.create);

router.get('/cluster/publications', publicationsResource.getList);
router.get('/cluster/publications/:publication', publicationsResource.get);
router.post('/cluster/publications', publicationsResource.add);
router.patch('/cluster/publications/:publication', publicationsResource.patch);
router.delete('/cluster/publications/:publication', publicationsResource.delete);

router.get('/cluster/subscriptions', subscriptionsResource.getList);
router.get('/cluster/subscriptions/:subscription', subscriptionsResource.get);
router.post('/cluster/subscriptions', subscriptionsResource.add);
router.patch('/cluster/subscriptions/:subscription', subscriptionsResource.patch);
router.delete('/cluster/subscriptions/:subscription', subscriptionsResource.delete);

router.get('/cluster/nodes', publicationsResource.getNodes);

router.get('/cluster/processors', processorsResource.getList);
router.get('/cluster/processors/:processors', processorsResource.get);
router.post('/cluster/processors', processorsResource.add);
router.delete('/cluster/processors/:processor', processorsResource.delete);

// Same as previous version
router.use(routerV1);

module.exports = router;
