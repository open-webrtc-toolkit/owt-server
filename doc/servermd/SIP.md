Gateway for SIP User Guide
--------------------------

# 1 Overview {#SIPsection1}

## 1.1 Introduction {#SIPsection1_1}
Welcome to the user guide for the Intel<sup>®</sup> Collaboration Suite for WebRTC (Intel<sup>®</sup> CS for WebRTC) Gateway for SIP. The gateway enables the WebRTC interoperability with traditional RTC systems.
This guide describes how to install and launch the gateway server for Session Initiation Protocol (SIP), and it provides details of the interfaces available in the gateway SDK for SIP. This guide includes these sections:

 - Section 1. Introduction and conventions used in this guide.
 - Section 2. Installing, launching, and stopping the Intel CS for WebRTC Gateway Server.
 - Section 3. Interfaces provided with the Intel CS for WebRTC Gateway SDK for SIP.

##1.2 Terminology {#SIPsection1_2}
This manual uses the following acronyms and terms:
  Abbreviation       |  Full Name
-------------|--------------
ADT | Android Developer Toolkit
API | Application programming interface
IDE | Integrated development environment
JS | JavaScript programming language
MCU | Multipoint control unit
MSML | Media server markup language
P2P | Peer-to-peer
QoS | Quality of service
ReST | Representational state transfer
RTC | Real-time communication
RTCP | RTP Control Protocol
RTP | Real Time Transport Protocol
SDK　| Software development kit
SDP | Session Description Protocol
SIP | Session Initiation Protocol
XMPP | Extensible Messaging and Presence Protocol
WebRTC | Web real-time communication

## 1.3 For more information  {#SIPsection1_3}
For more information, visit the following Web pages:

 - Intel HTML Developer Zone: https://software.intel.com/en-us/html5/tools
 - Intel Collaboration Suite for WebRTC:
    1. https://software.intel.com/webrtc
    2. https://software.intel.com/en-us/forums/webrtc
    3. https://software.intel.com/zh-cn/forums/webrtc
 - The Internet Engineering Task Force (IETF<sup>®</sup>) Working Group: http://tools.ietf.org/wg/rtcweb/
 - W3C WebRTC Working Group: http://www.w3.org/2011/04/webrtc/
 - WebRTC Open Project: http://www.webrtc.org

# 2 Gateway Installation for SIP  {#SIPsection2}
## 2.1 Introduction {#SIPsection2_1}
This section briefly lists the requirements, test environment, and dependencies for the gateway. This section then briefly explains how to configure, install, launch, and stop the gateway.
## 2.2 Requirements and test environment {#SIPsection2_2}
Table 2-1 describes the system requirements for installing the Intel CS for WebRTC Gateway for SIP.

**Table 2-1. Server requirements**
Application name | OS version
----|----
General Gateway Server | Ubuntu 14.04 LTS* 64-bit
GPU-accelerated Gateway Server | CentOS* 7.1

Install the OpenH264 library to support H.264 in the gateway for SIP when required, refer to the [Install the Cisco OpenH264 Library](#SIPsection2_2_1) section for more details.

> **Note**: The OpenH264 library is not required for GPU-accelerated Gateway.

If you want to set up gateway service powered by GPU-accelerated Gateway server, the following server side SDK needs to be installed on CentOS* 7.1:

 - Intel<sup>®</sup> Media Server Studio for Linux* version 2015 R6

Either Professional Edition or Community Edition is applicable. For download or installation instructions, please visit its website at https://software.intel.com/en-us/intel-media-server-studio.

The gateway for SIP has been tested in the environment listed in Table 2-2.

@htmlonly
<table class="doxtable">
<caption><b>Table 2-2. Test environment</b></caption>
<tbody>
<tr>
<th>Environment</th>
<th>Consists of</th>
</tr>
<tr>
<td>SIP server</td>
<td>Kamailio v4.1.3*</td>
</tr>
<tr>
<td>SIP client</td>
<td><ul><li>AVer EVC3000*</li>
<li>Jitsi v2.5* or later</li></td>
</tr>
<tr>
<td>Video codec</td>
<td>
VP8: <ul><li>Jitsi v2.5 or later</li></ul><br>
H264:
<ul><li>Aver EVC3000</li>
<li>Jitsi v2.5 or later</li></ul>
</td>
</tr>
<tr>
<td>Audio codec</td>
<td><ul><li>G.711*</li>
<li>Opus</li></ul></td>
</tr>
<tr>
<td>Web browser</td>
<td><ul><li>Google Chrome* v47</li>
<li>Firefox* v43</li></ul>
</td>
</tr>
</tbody>
</table>
@endhtmlonly

### 2.2.1 Install the Cisco OpenH264 Library {#SIPsection2_2_1}
The default H.264 library does not provide any media logic. To enable H.264 support in non GPU-accelerated gateway system, you must install the Cisco OpenH264 library.

Follow these steps to install the library:

 1. Go to the following URL and get the binary package: http://ciscobinary.openh264.org/libopenh264-1.4.0-linux64.so.bz2.

        curl –O http://ciscobinary.openh264.org/libopenh264-1.4.0-linux64.so.bz2

 2. Unzip this package with following command:

        bzip2 -d libopenh264-1.4.0-linux64.so.bz2

 3. Copy the lib file libopenh264-1.4.0-linux64.so to Release-<Version>/lib folder, and rename it to libopenh264.so.0 to replace the existing pseudo one.

## 2.3 Configure the gateway machine  {#SIPsection2_3}
To optimize the gateway server for best performance, we strongly recommend that you configure the system as shown in these simple steps:

1. Add or update the following lines in /etc/security/limits.conf, in order to set the maximum numbers of open files, running processes and maximum stack size to a large enough number:

        * hard nproc unlimited
        * soft nproc unlimited
        * hard nofile 163840
        * soft nofile 163840
        * hard stack 1024
        * soft stack 1024

   If you only want to target these setting to specific user or group rather than all with "*", please follow the configuration rules of the /etc/security/limits.conf file.

2. Make sure pam_limits.so appears in /etc/pam.d/login as following:

        session required pam_limits.so

   So that the updated limits.conf takes effect after your next login.

3. If you run gateway on CentOS, add or update the following two lines in /etc/security/limits.d/xx-nproc.conf as well:

        * soft nproc unlimited
        * hard nproc unlimited

4. Add or update the following lines in /etc/sysctl.conf:

        fs.file-max=200000
        net.core.rmem_max=16777216
        net.core.wmem_max=16777216
        net.core.rmem_default=16777216
        net.core.wmem_default=16777216
        net.ipv4.udp_mem = 4096 87380 16777216

5. Now run command /sbin/sysctl -p to activate the new configuration, or just restart your gateway machine.

6. You can run command "ulimit -a" to make sure the new setting in limits.conf is correct as you set.

## 2.4 Install the gateway package {#SIPsection2_4}
For general gateway server on Ubuntu, do as following:

        tar xf CS_WebRTC_Gateway_SIP.<Version>.tgz
        cd Release-<Version>/

For GPU-accelerated gateway server on CentOS, do as following:

        tar xf CS_WebRTC_Gateway_SIP.<Version>.hw.tgz
        cd Release-<Version>/

## 2.5 Launch the gateway server {#SIPsection2_5}
You can launch the gateway by itself, or you can launch the gateway and the sample Web application together.
To launch both the gateway and the sample web application, use the start-all command as shown here:

        bin/start-all.sh

To launch only the gateway, use this command:

        bin/daemon.sh start gateway

You can then access the sample application by launching your browser with URL: http://XXXXX:3001 or https://XXXXX:3004. Replace "XXXXX" with the IP address or machine name of the server on which Gateway and the sample application run.

**Note**: Latest Chrome browser versions from v47 force https access on WebRTC applications. You will got SSL warning page with default certificates, replace them with your own trusted ones.

## 2.6 Stop the gateway server {#SIPsection2_6}
Run the following commands to stop the gateway:

        bin/stop-all.sh

or stop it alone:

        bin/daemon.sh stop gateway

# 3 Gateway SDK for SIP　{#SIPsection3}
## 3.1 Introduction {#SIPsection3_1}
This section describes the interfaces provided in the Intel CS for WebRTC Gateway for SIP SDK.
### 3.1.1 Initialize your code so you can access the gateway {#SIPsection3_1_1}
Before you can use the gateway, you must initialize your HTML code to allow your files to connect to the gateway. To do this, copy and paste the following code into the head section of your HTML document:
~~~~~~~{.js}
    <script type="text/javascript" src="erizoclient.min.js"></script>
~~~~~~~
### 3.1.2 SDK special considerations {#SIPsection3_1_2}
 - When constructing a client, you should initialize the client with type ‘sip’ (explained in the next section of this guide).
 - Once connected to the gateway, the client must register with the SIP server. To do this, the client should send a custom message with authentication information to the gateway.
 - Once registered with the SIP server, the client must make a call or accept incoming calls before publishing its local stream to the gateway; otherwise, the stream publishing fails.

## 3.2 Objects {#SIPsection3_2}
The SDK includes two JavaScript objects: Client and Stream. Web applications use the objects to connect to the gateway and manage handling of audio/video streams and events.
~~~~{.js}
<script type = "text/javascript">
    var client = Erizo.Client({
        username: 'foo',
        role: 'bar',
        host: '127.0.0.1:8080',
        type: 'sip'
    });
</script>
~~~~
**Notes**:

 - host is the address of the gateway.
 - type indicates the gateway to connect to the corresponding service.

~~~~{.js}
<script type = "text/javascript" >
    var localStream = Erizo.Stream({
        audio: true,
        video: true
    });
</script>
~~~~
**Notes**:

 - The variable localStream is initialized locally and represents the audio/video stream from the local browser.
 - You should set either audio or video to true.
 - Remote streams are carried by event messages.

##3.3 Events {#SIPsection3_3}
Objects Client and Stream may trigger different events that can be handled by the application when corresponding event handlers are registered. Table 3-1 lists the events that may be triggered.
@htmlonly
<table class="doxtable">
<caption><b>Table 3-1. Events that may be triggered by Client and/or Stream</b></caption>
<tbody>
<tr>
<th>Category</th>
<th>Dispatcher</th>
<th>Event Type</th>
<th>Description</th>
</tr>
<tr>
<td rowspan="5">ClientEvent</td>
<td rowspan="5">Client</td>
<td>client-connected</td>
<td>Indicates the client has been successfully connected to the gateway.</td>
</tr>
<tr>
<td>client-disconnected</td>
<td>Indicates the client has been disconnected from the gateway.</td>
</tr>
<tr><td>client-joined</td>
<td>Indicates that there is an incoming call.</td></tr>
<tr><td>client-left</td>
<td>Indicates that a client has disconnected, and the call is closed.</td></tr>
<tr><td>message-received</td>
<td>Indicates that a new message has arrived to be handled.</td></tr>
<tr>
<td rowspan="10">StreamEvent</td>
<td rowspan="2">Stream</td>
<td>access-accepted </td>
<td>Indicates that the user has agreed to open the camera and/or microphone. </td>
</tr>
<tr>
<td>access-denied  </td>
<td>Indicates that the user has denied opening the camera and/or microphone.  </td>
</tr>
<tr>
<td rowspan="8">Client</td>
<td>stream-added </td>
<td>Indicates there is a new stream available. </td>
</tr>
<tr>
<td>stream-published</td>
<td>Indicates the local stream has been published.</td>
</tr>
<tr>
<td>video-paused</td>
<td>Indicates the video stream is paused.</td>
</tr>
<tr>
<td>video-play</td>
<td>Indicates the video stream is resumed</td>
</tr>
<tr>
<td>audio-paused</td>
<td>Indicates the audio stream is paused.</td>
</tr>
<tr>
<td>audio-play</td>
<td>Indicates the audio stream is resumed.</td>
</tr>
<tr>
<td>stream-removed </td>
<td>Indicates that an existing stream has been removed, and the call has been closed. </td>
</tr>
<tr>
<td>stream-subscribed </td>
<td>Indicates a stream has been subscribed. </td>
</tr>
</tbody>
</table>
@endhtmlonly
## 3.4 Methods {#SIPsection3_4}
### 3.4.1 Client {#SIPsection3_4_1}
####connect(onFailure) {#SIPsection3_4_1_1}
This function establishes a connection to the gateway. Once the connection is completed, a client-connected event is triggered.

onFailure are user-defined callbacks. If provided, these callbacks are invoked when connect() is failed.
####addEventListener(string, callback) {#SIPsection3_4_1_2}
This function registers a callback function as a handler for a corresponding event.
####on(string, callback) {#SIPsection3_4_1_3}
Equivalent to addEventListener.
####onMessage(callback) {#SIPsection3_4_1_4}
Shortcut of on(‘message-received’, callback).
####disconnect() {#SIPsection3_4_1_5}
This function disconnects from the gateway. A client-disconnected event is then triggered.
####publish(Stream, onFailure) {#SIPsection3_4_1_6}
This function publishes the local stream to the gateway. The stream must be initialized before publishing. Once initialization is complete, a stream-added event is triggered.

onFailure are user-defined callbacks. If provided, it would be invoked when publish() is failed.
####unpublish(Stream, onFailure) {#SIPsection3_4_1_7}
This function unpublishes the local stream. Once the stream is unpublished, a stream-removed event is triggered.

onFailure are user-defined callbacks. If provided, it would be invoked when unpublish() is failed.
####subscribe(Stream, onFailure) {#SIPsection3_4_1_8}
This function subscribes to a remote stream. Once subscription is complete, a stream-subscribed event is triggered

**Note:**	Make sure the function is subscribing to a remote stream.

onFailure are user-defined callbacks. If provided, it would be invoked when subscribe() is failed.
####unsubscribe(Stream, onFailure) {#SIPsection3_4_1_9}
This function unsubscribes to a remote stream.
**Note**:	Make sure the stream is a remote stream.

onFailure are user-defined callbacks. If provided, it would be invoked when unsubscribe() is failed.
####send(string/Object, onSuccess, onFailure) {#SIPsection3_4_1_10}
This function send a message to the gateway.

onSuccess and onFailure are user-defined callbacks. If provided, they would be invoked when send() is successful or failed respectively.
###3.4.2 Stream {#SIPsection3_4_2}
####init()#### {#SIPsection3_4_2_1}
This function initializes the local stream and tries to retrieve permission to access local video/audio devices. Once completed, an access-accepted or access-denied event is triggered.
####close()#### {#SIPsection3_4_2_2}
This function closes the local stream. If the stream has audio and/or video, the function also stops capturing camera/microphone. If the stream is published, the function also unpublishes the stream. If the stream is showing in an HTML element, the stream would be hidden after the function completes.
####show(string, Object) {#SIPsection3_4_2_3}
This function shows the video at the given element ID (string). The stream is then displayed on given HTML element. A volume slider can be optionally hidden by {speaker: false}.
####hide() {#SIPsection3_4_2_4}
This function stops displaying the video in the HTML page.
####getId() {#SIPsection3_4_2_5}
This function returns the stream ID assigned by gateway. For the local stream, it returns undefined if the stream has not been published; once published, the stream ID is updated by the gateway.
####addEventListener(string, callback) {#SIPsection3_4_2_6}
This function registers a callback function as a handler for a corresponding event.
####on(string, callback) {#SIPsection3_4_2_7}
This function is equivalent to addEventListener.
###3.4.3 Miscellaneous {#SIPsection3_4_3}
####Logger.setLogLevel(Number) {#SIPsection3_4_3_1}
This function sets the log level. There are 6 log levels:

 - Logger.DEBUG (default)
 - Logger.TRACE
 - Logger.INFO
 - Logger.WARNING
 - Logger.ERROR
 - Logger.NONE
##3.5 Call procedure {#SIPsection3_5}
###3.5.1 Register to a SIP server {#SIPsection3_5_1}
To register to a SIP server, the client should send a message to the gateway with type ‘login’ and a proper payload:

~~~~{.js}
client.send({
    type: 'login',
    payload: {
        display_name: "user1",
        sip_uri: "user1",
        sip_password: "password",
        sip_server: "127.0.0.1",
    }
}, function() { //...
});
~~~~

The sip_uri is the username of a SIP user. The gateway constructs the final SIP URI from the payload.
###3.5.2 Make a call {#SIPsection3_5_2}
To make a call with peer, the client should send a message with type ‘makeCall’:

~~~~{.js}
function makeCall(option) {
    client.send({
        type: 'makeCall',
        payload: {
            calleeURI: option.calleeURI,
            audio: option.audio,
            video: option.video
        }
    }, function(msg) {
        localStream.init();
    });
}
~~~~

To detect a `'callAbort’` when the peer is denied a calling, the client should listen for a specified message:

~~~~{.js}
client.onMessage(function(evt) {
    var msg = evt.attr;
    if (typeof msg === 'object' &&
        msg !== null &&
        msg.type === 'callAbort') {
        // call cancelled
        return;
    }
    // other msg handling
});
~~~~
###3.5.3 Accept a call {#SIPsection3_5_3}
To accept a call from the peer, the client should send a message with type `'acceptCall'`:
~~~~{.js}
function acceptCall() {
    client.send({
        type: 'acceptCall',
        payload: {}
    }, function(msg) {
        localStream.init();
    });
}
~~~~
###3.5.4 Reject a call {#SIPsection3_5_4}
To reject a call from the peer, the client should send a message with type `'rejectCall'`:
~~~~{.js}
function rejectCall() {
    client.send({
        type: 'rejectCall',
        payload: {}
    }, function(msg) {});
}
~~~~
##3.6 Code snips {#SIPsection3_6}
Here is a snip from the sample application. For details about this code snip, refer to the sample application provided with the SDK.

~~~~{.js}
<script type="text/javascript" src="erizoclient.js"></script>
<script type="text/javascript">
L.Logger.setLogLevel(L.Logger.INFO);

function makeCall(option) {
    client.send({
        type: 'makeCall',
        payload: {
            calleeURI: option.calleeURI,
            audio: option.audio,
            video: option.video
        }
    }, function(msg) {
        localStream.init();
    });
}

function acceptCall() {
    client.send({
        type: 'acceptCall',
        payload: {}
    }, function(msg) {
        localStream.init();
    });
}

function rejectCall() {
    client.send({
        type: 'rejectCall',
        payload: {}
    }, function(msg) {});
}

client.on("client-connected", function(e) {
    client.send({
        type: 'login',
        payload: accinfo
    }, function() { //...
    });
});

client.on("stream-subscribed", function(e) {
    var stream = e.stream;
    var div = document.createElement('div');
    div.setAttribute("style", "width: 320px; height: 240px;");
    div.setAttribute("id", "remote" + stream.getId());
    document.getElementById('conference').appendChild(div);
    stream.show("remote" + stream.getId());
});

client.on("stream-added", function(e) {
    //...
});

client.on("stream-removed", function(e) {
    //...
});

client.on('client-joined', function(e) {
    //...
});

client.on('client-left', function(e) {
    //...
});

client.on('client-id', function(e) {
    //...
});

client.onMessage(function(e) {
    //...
});

client.connect(function(err) {
    //...
});

localStream.on("access-accepted", function() {
    localStream.show("myVideo");
    client.publish(localStream, {
            maxVideoBW: maxVideoBW
        },
        function(err) {
            L.Logger.error(err);
        });
});

localStream.init();
</script>
~~~~
