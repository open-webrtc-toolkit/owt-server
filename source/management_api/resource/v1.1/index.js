// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * Router for v1.1 interfaces
 */
'use strict';
var express = require('express')
var router = express.Router()
var streamsResource = require('./streamsResource');
var cascadingResource = require('./cascadingResource');

//Stream(including external streaming-in) management
router.get('/rooms/:room/streams', streamsResource.getList);
router.get('/rooms/:room/streams/:stream', streamsResource.get);
router.patch('/rooms/:room/streams/:stream', streamsResource.patch);
router.delete('/rooms/:room/streams/:stream', streamsResource.delete);
router.post('/rooms/:room/streaming-ins', streamsResource.addStreamingIn);//FIXME: Validation on body.type === 'streaming' is needed.
router.delete('/rooms/:room/streaming-ins/:stream', streamsResource.delete);

//Cluster cascading management
router.post('/rooms/:room/eventCascading', cascadingResource.startEventCascading);

module.exports = router
