# Stream service

## Overview
Stream service provides functionality to control streams and subscriptions directly in OWT server. Related REST interfaces are added in management API.

## Architecture and dataflow

![data flow](./pics/)


## REST API

The following REST APIs can be called in the same way as management REST APIs (). The url prefix for stream service is "/cluster/".

### Publication
A publication represents the stream published by user. 

Data format of publication:

    {
        "id": "publicationId(string)",
        "type": "webrtc|streaming|...",
        "source": {
            "audio": [
                {"id": "trackId", "format": {/*Audio format*/}, "info": {/*info*/}}
            ],
            "video": [
                {
                    "id": "trackId",
                    "format": {/*Video format*/},
                    "parameters": {/*Video parameters*/},
                    "info": {/*Track info*/}
                }
            ],
            "data": [
                {"id": "trackId"}
            ]
        },
        "info": {
            "owner": "participantId for WebRTC streams",
            "attributes": "customized attributes"   
        },
        "locality": {"node": "actual node", "agent": "node manager"},
        "domain": "string used to describe scope like room"
    }

POST hostname:port/v1.1/cluster/publications - publish
GET hostname:port/v1.1/cluster/publications - list publications
GET hostname:port/v1.1/cluster/publications/:pubId - get publication info with given ID
DELETE hostname:port/v1.1/cluster/publications/:pubId - unpublish


### Subscription
A subscription represents stream subscribed by user.

Data format of subscription:

    {
        "id": "subscriptionId(string)",
        "type": "webrtc|streaming|recording|...",
        "sink": {
            "audio": [
                {
                    "id": "trackId",
                    "from": "fromSourceId",
                    "format": {/*Audio format*/},
                    "info": {/*info*/}
                }
            ],
            "video": [
                {
                    "id": "trackId",
                    "from": "fromSourceId",
                    "format": {/*Video format*/},
                    "parameters": {/*Video parameters*/},
                    "info": {/*Track info*/}
                }
            ],
            "data": [
                {"id": "trackId", "from": "fromSourceId"}
            ]
        },
        "info": {
            "owner": "participantId for WebRTC streams",
            "attributes": "customized attributes"   
        },
        "locality": {"node": "actual node", "agent": "node manager"},
        "domain": "string used to describe scope like room"
    }

POST hostname:port/v1.1/cluster/subscriptions - subscribe
GET hostname:port/v1.1/cluster/subscriptions - list subscriptions
GET hostname:port/v1.1/cluster/subscriptions/:subId - get subscription info with given ID
DELETE hostname:port/v1.1/cluster/subscriptions/:subId - unsubscribe

### Processor
A processor takes streams as input and produces new streams as output, such as mixer and transcoder.

Data format of processor:

    {
        "id": "processorId(string)",
        "type": "vmixer|vtranscoder|...",
        "inputs": {
            "audio": [
                {"id": "inputId", "from": "fromSourceId"}
            ],
            video: [],
            data: [],
        },
        outputs: {
            audio: [
                {"id": "outputId", "info": "outputInfo"}
            ],
            video: [],
            data: [],
        },
        info: {

        },
        "locality": {"node": "actual node", "agent": "node manager"},
        "domain": "string used to describe scope like room"
    }

### Node
A node represents a working process of specific type in the cluster.

Data format of node:

    {
        "id": "nodeID",
        "agent": "ID of its manager agent",
        "type": "webrtc|...",
        "pubs": ["list of publication IDs on this node"],
        "subs": ["list of subscription IDs on this node"]
    }

GET hostname:port/v1.1/cluster/nodes - list nodes
