// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * Router for v1 interfaces
 */
'use strict';
var express = require('express')
var router = express.Router()

var roomsResource = require('./roomsResource');
var roomResource = require('./roomResource');
var tokensResource = require('./tokensResource');
var participantsResource = require('./participantsResource');
var streamsResource = require('./streamsResource');
var streamingOutsResource = require('./streamingOutsResource');
var recordingsResource = require('./recordingsResource');
var sipcallsResource = require('./sipcallsResource');
var analyticsResource = require('./analyticsResource');
var cascadingResource = require('./cascadingResource');

//Room management
router.post('/rooms', roomsResource.createRoom); //FIXME: The definition of 'options' needs to be refined.
router.get('/rooms', roomsResource.represent); //FIXME: The list result needs to be simplified.
router.get('/rooms/:room', roomResource.represent); //FIXME: The detailed format of a complete room configuration data object needs to be refined.
router.delete('/rooms/:room', roomResource.deleteRoom);
router.put('/rooms/:room', roomResource.updateRoom);
router.patch('/rooms/:room', (req, res, next) => next(new e.routerError('Not implemented'))); //FIXME: To be implemented.

//Room checker for all the sub-resources
router.use('/rooms/:room/*', roomResource.validate);

//Participant management
router.get('/rooms/:room/participants', participantsResource.getList);
router.get('/rooms/:room/participants/:participant', participantsResource.get);
router.patch('/rooms/:room/participants/:participant', participantsResource.patch);
router.delete('/rooms/:room/participants/:participant', participantsResource.delete);

//Stream(including external streaming-in) management
router.get('/rooms/:room/streams', streamsResource.getList);
router.get('/rooms/:room/streams/:stream', streamsResource.get);
router.patch('/rooms/:room/streams/:stream', streamsResource.patch);
router.delete('/rooms/:room/streams/:stream', streamsResource.delete);
router.post('/rooms/:room/streaming-ins', streamsResource.addStreamingIn);//FIXME: Validation on body.type === 'streaming' is needed.
router.delete('/rooms/:room/streaming-ins/:stream', streamsResource.delete);

//External streaming-out management
router.get('/rooms/:room/streaming-outs', streamingOutsResource.getList);
router.post('/rooms/:room/streaming-outs', streamingOutsResource.add);//FIXME: Validation on body.type === 'streaming' is needed.
router.patch('/rooms/:room/streaming-outs/:id', streamingOutsResource.patch);
router.delete('/rooms/:room/streaming-outs/:id', streamingOutsResource.delete);

//Server side recording management
router.get('/rooms/:room/recordings', recordingsResource.getList);
router.post('/rooms/:room/recordings', recordingsResource.add);//FIXME: Validation on body.type === 'recording' is needed.
router.patch('/rooms/:room/recordings/:id', recordingsResource.patch);
router.delete('/rooms/:room/recordings/:id', recordingsResource.delete);

//Sip call management
router.get('/rooms/:room/sipcalls', sipcallsResource.getList);
router.post('/rooms/:room/sipcalls', sipcallsResource.add);
router.patch('/rooms/:room/sipcalls/:id', sipcallsResource.patch);
router.delete('/rooms/:room/sipcalls/:id', sipcallsResource.delete);

//Analytic management
router.get('/rooms/:room/analytics', analyticsResource.getList);
router.post('/rooms/:room/analytics', analyticsResource.add);
router.delete('/rooms/:room/analytics/:id', analyticsResource.delete);

//Create token.
router.post('/rooms/:room/tokens', tokensResource.create);

//Cluster cascading management
router.post('/rooms/:room/cascading', cascadingResource.startCascading);
router.get('/rooms/:room/bridges', cascadingResource.getBridges);

module.exports = router
