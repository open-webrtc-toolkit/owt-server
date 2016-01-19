Conference Server User Guide
----------------------

# 1 Overview {#Conferencesection1}
## 1.1 Introduction {#Conferencesection1_1}
Welcome to the Conference Server User Guide for the Intel<sup>®</sup> Collaboration Suite for WebRTC (Intel<sup>®</sup> CS for WebRTC). This guide describes how to install and configure the Intel CS for WebRTC multipoint control unit (MCU). This guide also explains how to install and launch the peer server.

The Intel CS for WebRTC Conference Server provides an efficient WebRTC-based video conference service that scales a single WebRTC stream out to many endpoints. The following list briefly explains the purpose of each section in this guide:

 - Section 1. Introduction and conventions used in this guide.
 - Section 2. Installing and configuring the MCU.
 - Section 3. Installing the MCU sample application server.
 - Section 4. Installing and launching the peer server.

Installation requirements and dependencies for the MCU, sample application server, and peer server are described in their associated sections.

##1.2 Terminology {#Conferencesection1_2}
This manual uses the following acronyms and terms:

  Abbreviation       |  Full Name
-------------|--------------
ADT|Android Developer Toolkit
API|Application programming interface
GPU|Graphics processing unit
IDE|Integrated development environment
JS|JavaScript programming language
MCU|Multipoint control unit
MSML|Media server markup language
P2P|Peer-to-peer
QoS|Quality of service
ReST|Representational state transfer
RTC|Real-time communication
RTCP|RTP Control Protocol
RTSP|Real Time Streaming Protocol
RTP|Real Time Transport Protocol
SDK|Software development kit
SDP|Session Description Protocol
SIP|Session Initiation Protocol
XMPP|Extensible Messaging and Presence Protocol
WebRTC|Web real-time communication

##1.3 For more information {#Conferencesection1_3}
For more information, visit the following Web pages:

 - Intel HTML Developer Zone:
https://software.intel.com/en-us/html5/tools
 - Intel Collaboration Suite for WebRTC:
https://software.intel.com/webrtc
https://software.intel.com/en-us/forums/webrtc
https://software.intel.com/zh-cn/forums/webrtc
 - The Internet Engineering Task Force (IETF<sup>®</sup>) Working Group:
http://tools.ietf.org/wg/rtcweb/
 - W3C WebRTC Working Group:
http://www.w3.org/2011/04/webrtc/
 - WebRTC Open Project:
http://www.webrtc.org

# 2 MCU Installation {#Conferencesection2}

## 2.1 Introduction {#Conferencesection2_1}

This section describes the system requirements for installing the MCU server, and the compatibility with its client.

> **Note**:	Installation requirements for the peer server are described in <a href="#Conferencesection5">section 5</a> of this guide.

## 2.2 Requirements and compatibility {#Conferencesection2_2}

Table 2-1 describes the system requirements for installing the MCU server. Table 2-2 gives an overview of MCU compatibility with the client.

**Table 2-1. Server requirements**
Application name|OS version
-------------|--------------
General MCU server|Ubuntu 14.04 LTS* 64-bit
GPU-accelerated MCU server|CentOS* 7.1

H.264 support in MCU system requires the deployment of OpenH264 library, see [Deploy Cisco OpenH264* Library section](#Conferencesection2_3_4) for more details.

> **Note**: OpenH264 library is not required for GPU-accelerated MCU when forward RTSP stream subscription is not used.
If you want to set up video conference service powered by GPU-accelerated MCU server, the following server side SDK needs to be installed:

 - Intel<sup>®</sup> Media Server Studio for Linux* version 2015 R6

Either Professional Edition or Community Edition is applicable. For download or installation instructions, please visit its website at https://software.intel.com/en-us/intel-media-server-studio.

 **Table 2-2. Client compatibility**
Application Name|Google Chrome* 47|Mozilla Firefox* 43|Microsoft Internet Explorer* (IE) 9， 10， 11|Intel CS for WebRTC Client SDK for Android
--------|--------|--------|--------|--------
MCU Client|YES|YES|YES|YES
Management Console|YES|YES|N/A|N/A

## 2.3 Install the MCU server {#Conferencesection2_3}
This section describes the dependencies and steps for installing the MCU.
### 2.3.1 Dependencies {#Conferencesection2_3_1}
**Table 2-3. MCU Dependencies**
Name|Version|Remarks
--------|--------|--------
Node.js |4.*|Website: http://nodejs.org/
Node modules|Specified|N/A
MongoDB| 2.4.9 |Website: http://mongodb.org
System libraries|Latest|N/A

All dependencies, except system libraries and node, are provided with the release package.

All essential system libraries are installed when you install the MCU package using the Ubuntu or CentOS's package management system.

Regarding Node.js*, make sure it's installed in your system prior to installing the MCU. We recommend version 4.1.2. Refer to http://nodejs.org/ for the details and installation.

Before installing the MCU, make sure your login account has sys-admin privileges; i.e. the ability to execute `sudo`.

### 2.3.2 Configure the MCU server machine {#Conferencesection2_3_2}

In order for the MCU server to deliver the best performance on video conferencing, the following system configuration is recommended:

1. Add or update the maximum numbers of open files to a large enough number by adding the following two lines to /etc/security/limits.conf:

        * hard nofile 163840
        * soft nofile 163840

   If you only want to target these setting to specific user or group rather than all with "*", please follow the configuration rules of the /etc/security/limits.conf file.

2. Make sure pam_limits.so appears in /etc/pam.d/login as following:

        session required pam_limits.so

   So that the updated limits.conf takes effect after your next login.
3. Add or update the following lines to /etc/sysctl.conf:

        fs.file-max=200000
        net.core.rmem_max=16777216
        net.core.wmem_max=16777216
        net.core.rmem_default=16777216
        net.core.wmem_default=16777216
        net.ipv4.udp_mem = 4096 87380 16777216

4. Now run command /sbin/sysctl -p to activate the new configuration, or just restart your MCU machine.
5. You can run command "ulimit -a" to make sure the new setting in limits.conf is correct as you set.f

### 2.3.3 Install the MCU package {#Conferencesection2_3_3}

In the server machine, un-archive the package file first, and then invoke init.sh to initialize the package.
~~~~~~{.js}
tar xf CS_WebRTC_Conference_Server_MCU.v<Version>.tgz
cd Release-<Version>/
bin/init.sh --deps [--hardware]
~~~~~~
> **Note**: 	If you have already installed the required system libraries, you can omit the **--deps** option. What's more, if you want to run the GPU-accelerated MCU, add **--hardware** to the init command.
### 2.3.4 Deploy Cisco OpenH264* Library {#Conferencesection2_3_4}
The default H.264 library installed is a pseudo one without any media logic. To enable H.264 support in non GPU-accelerated MCU system, the deployment of Cisco OpenH264 library is required; follow these steps:
1. Go to the following URL and get the binary package:
    http://ciscobinary.openh264.org/libopenh264-1.4.0-linux64.so.bz2.

        curl –O http://ciscobinary.openh264.org/libopenh264-1.4.0-linux64.so.bz2

2. Unzip this package with following command:

        bzip2 -d libopenh264-1.4.0-linux64.so.bz2

3. Copy the lib file libopenh264-1.4.0-linux64.so to Release-<Version>/lib folder, and rename it to libopenh264.so.0 to replace the existing pseudo one.

4. Edit etc/woogeen_config.js file under Release-<Version> folder, set the value of key config.erizo.openh264Enbaled to be true.

### 2.3.5 Use your own certificate {#Conferencesection2_3_5}

The default certificate (certificate.pfx) for the MCU is located in the Release-<Version>/cert folder.
When using HTTPS and/or secure socket.io connection, you should use your own certificate for each server. First you should edit etc/woogeen_config.js to provide the path of each certificate for each server, under the key config.XXX.keystorePath. E.g., config.nuve.keystorePath is for nuve HTTPS server. See below table for details. If etc/woogeen_config.js does not exist, use `bin/init.sh` to create it.

We use PFX formatted certificates in MCU. See https://nodejs.org/api/tls.html for how to generate a self-signed certificate by openssl utility. We recommend using 2048-bits private key for the certificates.

After editing the configuration file, you should run `bin/initcert.js all` to input your passphrases for the certificates, which would then store them in an encrypted file. Be aware that you should have node binary in your shell's $PATH to run the JS script. bin/initcert.js accepts ‘nuve', ‘erizo' and 'erizoController' (or ‘all' for all) as parameters to update the passphrase for a certain certificate.

 **Table 2-4. MCU certificates configuration**
|  |key in configuration file|initcert.js parameter|
|--------|--------|--------|
| nuve HTTPS | config.nuve.keystorePath | nuve |
| erizoController secured Socket.io | config.erizoController.keystorePath | erizoController |
| DTLS-SRTP | config.erizo.keystorePath | erizo |

For MCU sample application's certificate configuration, please follow the instruction file 'README.md' located at Release-<Version>/extras/basic_example/.

### 2.3.6 Launch the MCU server as single node {#Conferencesection2_3_6}
To launch the MCU server on one machine, follow steps below:

1. Run the following commands to start the MCU:

        cd Release-<Version>/
        bin/start-all.sh

2. To verify whether the server started successfully, launch your browser and connect to the MCU server at https://XXXXX:3004. Replace XXXXX with the IP address or machine name of your MCU server.

**Note** that the procedures in this guide use the default room in the sample.

### 2.3.7 Stop the MCU server {#Conferencesection2_3_7}
Run the following commands to stop the MCU:

        cd Release-<Version>/
        bin/stop-all.sh

### 2.3.8 Set up the General MCU cluster {#Conferencesection2_3_8}
Follow the steps below to set up an MCU cluster which comprises multiple worker nodes:
1. Make sure you have installed the MCU package on each machine before launching the cluster which has been described in section [Install the MCU package](#Conferencesection2_3_3).
2. Choose a primary machine.
3. Rou can run nuve, mcu controller and the application on the primary machine without any MCU worker node by running the following commands:

        cd Release-<Version>/
        bin/daemon.sh start nuve
        bin/daemon.sh start mcu
        bin/daemon.sh start app

4. Choose a slave machine.
5. Edit the configuration file
Release-<Version>/etc/woogeen_config.js on the slave machines:

    i. Make sure the config.rabbit.port and config.rabbit.host point to the RabbitMQ server.

6. Run the following commands to launch the MCU worker node on this slave machine:

        cd Release-<Version>/
        bin/daemon.sh start agent

7. Repeat step 4 to 6 to launch as many MCU slave machines as you need.
### 2.3.9 Stop the General MCU cluster {#Conferencesection2_3_9}
To stop the MCU cluster, do as following:
1. Run the following commands on primary node to stop the application, MCU controller and nuve.

        cd Release-<Version>/
        bin/daemon.sh stop app
        bin/daemon.sh stop mcu
        bin/daemon.sh stop nuve
2. Run the following commands on slave node to stop MCU worker node.

        cd Release-<Version>/
        bin/daemon.sh stop agent

### 2.3.10 Set up the GPU-accelerated MCU cluster {#Conferencesection2_3_10}
To better fullfill the large scale video conference deployment and utilize Intel media acceleration platforms, like Intel Visual Compute Accelerator(VCA), Intel CS for WebRTC architecture are evolving to be more distributed. GPU-accelerated MCU was firstly got upgrade to this new distributed architecture, and it will further expand to the General MCU version on next release.

 **Table 2-5. Distributed MCU components**
Component Name|Deployment Number|Responsibility
--------|--------|--------
nuve|1|The manager of the MCU, keeping the configurations of all rooms, generating and verifying the tokens, allocating and deallocating accessing nodes and media processing nodes for conferences
portal|1 or many|The signaling server and room controller, handling service requests from and responding to the clients
webrtc-agent|1 or many|This agent spawning webrtc accessing nodes which establish peer-connections with webrtc clients, receive media streams from and send media streams to webrtc clients
rtsp-agent|1 or many|This agent spawning rtsp accessing nodes which pull rtsp streams from rtsp sources and push rtsp streams to rtsp destinations
recording-agent|1 or many|This agent spawning recording nodes which record the specified audio/video streams to permanent storage facilities
audio-agent|1 or many|This agent spawning audio processing nodes which perform audio transcoding and mixing
video-agent|1 or many|This agent spawning video processing nodes which perform video transcoding and mixing
app|0 or 1|The sample web application for reference, users should use their own application server

Follow the steps below to set up an GPU-accelerated MCU cluster:
1. Make sure you have installed the MCU package on each machine before launching the cluster which has been described in section [Install the MCU package](#Conferencesection2_3_3).
2. Choose a primary machine.
3. You can run MCU manager nuve and the application on the primary machine without any MCU worker node with following commands:

        cd Release-<Version>/
        bin/daemon.sh start nuve
        bin/daemon.sh start app

4. Choose a slave machine.
5. Edit the configuration file
Release-<Version>/etc/woogeen_config.js on the slave machines:

    1) Make sure the config.rabbit.port and config.rabbit.host point to the RabbitMQ server.
    2) For portal node, make sure the config.erizoController.networkInterface is specified to the correct network interface which the clients’ signaling and control messages are expected to connect through.
    3) For any agent node, make sure the config.erizoAgent.networkInterface is specified to the correct network interface which the media streams are expected to be transmitted.

6. Run the following commands to launch one or multiple MCU worker nodes on this slave machine:

        cd Release-<Version>/
        bin/daemon.sh start start portal/webrtc-agent/rtsp-agent/audio-agent/video-agent/recording-agent

7. Repeat step 4 to 6 to launch as many MCU slave machines as you need.

### 2.3.11 Stop the GPU-accelerated MCU cluster {#Conferencesection2_3_11}

To stop the GPU-accelerated MCU cluster, do as following:
1. Run the following commands on primary node to stop the application and MCU manager nuve:

        cd Release-<Version>/
        bin/daemon.sh stop app
        bin/daemon.sh stop nuve
2. Run the following commands on slave node to stop MCU worker nodes installed on this node:

        cd Release-<Version>/
        bin/daemon.sh stop portal/webrtc-agent/rtsp-agent/audio-agent/video-agent/recording-agent

## 2.4 Security Recommendations {#Conferencesection2_4}
Intel Corporation does not host any conference cluster/service, instead, the entire suite is provided so you can build your own video conference system and host your own server cluster.

Customers must be familiar with industry standards and best practices for deploying server clusters. Intel Corporation assumes no responsibility for loss caused from potential improper deployment and mismanagement.

The following instructions are provided only as recommendations regarding security best practices and by no means are they fully complete:

1. For the key pair access on MCU server, make sure only people with high enough privilege can have the clearance.
2. Regular system state audits or system change auto-detection. For example, MCU server system changes notification mechanism by third-party tool.
3. Establish policy of file based operation history for the tracking purpose.
4. Establish policy disallowing saving credentials for remote system access on MCU server.
5. Use a policy for account revocation when appropriate and regular password expiration.
6. Use only per user account credentials, not account groups on MCU server.
7. Automated virus scans using approved software on MCU server.
8. Establish policy designed to ensure only allowed use of the server for external connections.
9. MCU server should install only approved software and required components, and verify default features status. For example, only enable what are needed.
10. Use a firewall and close any ports not specifically used on MCU server.
11. Run a vulnerability scan regularly for checking software updates against trusted databases, and monitor publically communicated findings and patch MCU system immediately.
12. Configure appropriate connection attempt timeouts and automated IP filtering, in order to do the rejection of unauthenticated requests.
13. If all possible, use or develop an automated health monitoring component of server service availability on MCU server.
14. Use a log analyzer solution to help detect attack attempts.
15. During server development, avoid processing of unexpected headers or query string variables, and only read those defined for the API and limit overall message size.
16. Avoid excessive error specificity and verbose details that reveal how the MCU server works.
17. During server and client development, suggest running some kind of security development lifecycle/process and conduct internal and external security testing and static code analysis with tools by trusted roles.
18. If all possible, backup all the log data to a dedicated log server with protection.
19. Deploy MCU cluster(Manager + Workers) inside Demilitarized Zone(DMZ) area, utilize external firewall A to protect them against possible attacks, e.g. DoS attack; Deploy Mongo DB and RabbitMQ behind DMZ, configure internal firewall B to make sure only cluster machines can connect to RabbitMQ server and access to MongoDB data resources.

**Figure 2-1. Security Recommendations Picture**
![Security Recommendations Picture](./pic/deploy.png)

## 2.5 FAQ {#Conferencesection2_5}
1. Sudden low volume when connecting Chrome on Windows to MCU
    **Resolution**:

    Both the Chrome browser and Windows system itself can reduce the volume during a connection to the MCU server. To resolve this issue, disable the following Communications feature found in the Sound Settings using the Windows Control Panel.

    **Figure 2-2. Sound Settings**
    ![Sound Settings](./pic/soundsettings.png)
2. Failed to start MCU server, and receive the following error message: "child_process.js:948; throw errnoException(process._errno, 'spawn'); Error: spawn EMFILE"

    **Resolution**:

    Use the proper Node.js version as outlined in the [Dependencies](#Conferencesection2_3_1) section.

3. Failed to start MCU server, and receive the following error message: "Creating superservice in localhost/nuvedb SuperService ID: Sat Feb 7 19:10:32.456 TypeError: Cannot read property '_id' of null SuperService KEY: Sat Feb  7 19:10:32.479 TypeError: Cannot read property 'key' of null"

    **Resolution**:

    Use the proper MongoDB version as outlined in the Dependencies section.
4. Run into network port conflicts on MCU, and probably some error message like "net::ERR_CONNECTION_TIMED_OUT" or "ERROR: server connection failed: connection_error"

    **Resolution**:

    In the MCU server, the following default ports have been assigned for MCU usage: 5672 (configurable), 8080 (configurable), 3000. Make sure they are always available for MCU. Also, in order to configure the two configurable ports above to a value smaller than the 1024 limitation, use the following command to enable it:

        sudo setcap cap_net_bind_service=+ep $(which node)

    If you are still not able to bypass the 1024 port limitation, remember to put the **MCU library path into /etc/ld.so.conf.d**.
# 3 MCU Management Console Brief Guide {#Conferencesection3}
## 3.1 Introduction {#Conferencesection3_1}
The MCU Management Console is the frontend console to manage the MCU server. It is built with MCU's server-side APIs and it provides the management interface to MCU administrators.
## 3.2 Access {#Conferencesection3_2}
Once you have launched MCU servers, you can then access the console via a browser at http://XXXX:3000/console/. You will be asked for your the service-id and service-key in order to access the service.

After inputting your service-id and service-key in the dialog prompt, choose ‘remember me' and click ‘save changes' to save your session. If you want to switch to another service, click the ‘profile' button on the upper-right corner to get in this dialog prompt and do the similar procedure again. If you want log out the management console, click the red ‘clear cookie' button in the dialog prompt.

If you have not launched MCU severs, you should launch the nuve server before accessing the management console:

        cd Release-<Version>/
        bin/daemon.sh start nuve

## 3.3 Source Code {#Conferencesection3_3}
The source code of the management console is in Release-<Version>/nuve/nuveAPI/public/.
## 3.4 Service Management {#Conferencesection3_4}
Only super service user can access service management, in the ‘overview' tab to create or delete services. Note that super service cannot be deleted.
## 3.5 Room Management {#Conferencesection3_5}
Any service user can do room management inside the service, including create, delete or modify rooms.

Specifically for modifying rooms, user can choose room mode, room publish limit, user limit and media mixing configuration (only for hybrid mode) for its own preference. When room mode is hybrid, user can define a configuration set for media mixing: resolution, bitrate, background color, layout, etc. For VAD, set avCoordinated to true to enable VAD in the room. Enabling multistreaming can let MCU generate two or more mixed streams with different resolutions to fullfill different devices. For layout, use can choose a base layout template and customize its own preferred ones, which would be combined as a whole for rendering mixed video.

**Note that** if base layout is set to 'void', user must input customized layout for the room, otherwise the video layout would be treated as invalid. Read 3.5.1 for details of customized layout. maxInput indicates the maximum number of video frame inputs for the video layout definition.

### 3.5.1 Customized video layout {#Conferencesection3_5_1}
The MCU server supports the mixed video layout configuration which is compliant with RFC5707 Media Server Markup Language (MSML).
A valid customized video layout should be a JSON string which represents an array of video layout definition.
The following example shows the details:

**Figure 3-1. Example Layout**
![Example layout](./pic/layout.jpg)

~~~~~~{.js}
// Video layout for the case of 1 input or 6 inputs
[
  {
    "region": [
      {
        "id": "1",
        "left": 0,
        "top": 0,
        "relativesize": 1,
      }
    ]
  },
  {
    "region": [
      {
        "id": "1",
        "left": 0,
        "top": 0,
        "relativesize": 0.667,
      },
      {
        "id": "2",
        "left": 0.667,
        "top": 0,
        "relativesize": 0.333,
      },
      {
        "id": "3",
        "left": 0.667,
        "top": 0.333,
        "relativesize": 0.333,
      },
      {
        "id": "4",
        "left": 0.667,
        "top": 0.667,
        "relativesize": 0.333,
      },
      {
        "id": "5",
        "left": 0.333,
        "top": 0.667,
        "relativesize": 0.333,
      },
      {
        "id": "6",
        "left": 0,
        "top": 0.667,
        "relativesize": 0.333,
      }
    ]
  }
]
~~~~~~
Each "region" defines video panes that are used to display participant video streams.

Regions are rendered on top of the root mixed stream. "id" is the identifier for each video layout region.

The size of a region is specified relative to the size of the root mixed stream using the "relativesize" attribute.

Regions are located on the root window based on the value of the position attributes "top" and "left".  These attributes define the position of the top left corner of the region as an offset from the top left corner of the root mixed stream, which is a percent of the vertical or horizontal dimension of the root mixed stream.

## 3.6 Runtime Configuration {#Conferencesection3_6}
Only super service user can access runtime configuration. Current management console implementation just provides the MCU cluster runtime configuration viewer.

## 3.7 Special Notes {#Conferencesection3_7}
Due to server-side APIs' security consideration, API calls from a service should be synchronized and sequential. Otherwise, server may respond with 401 if the request's timestamp is earlier than the previous request. As a result, API calls of single service are from different machines while these machines' time are not calibrated, one or another machine would encounter server response with 401 since their timestamps are messed.
# 4 MCU Sample Application Server User Guide  {#Conferencesection4}
## 4.1 Introduction {#Conferencesection4_1}
The MCU sample application server is a Web application demo that shows how to host audio/video conference services powered by the Intel CS for WebRTC MCU. The sample application server is based on MCU runtime components. Refer to [Section 2](#Conferencesection2) of this guide, for system requirements and launch/stop instructions.

This section explains how to start a conference and then connect to a conference using different qualifiers, such as a specific video resolution.

## 4.2 Start a conference through the MCU sample application server {#Conferencesection4_2}
These general steps show how to start a conference:

1. Start up the MCU server components.
2. Launch your Google Chrome* browser from the client machine.
3. Connect to the MCU sample application server at: http://XXXXX:3001 or https://XXXXX:3004. Replace XXXXX with the IP address or machine name of the MCU sample application server.

   **Note**: Latest Chrome browser versions from v47 force https access on WebRTC applications. You will got SSL warning page with default certificates, replace them with your own trusted ones.
4. Start your conference with this default room created by the sample application server.

### 4.2.1 Connect to an MCU conference with specific room {#Conferencesection4_2_1}
You can connect to a particular conference room. To do this, simply specify your room ID via a query string in your URL: room.
For example, connect to the MCU sample application server XXXXX with the following URL:

        https://XXXXX:3004/?room=some_particular_room_id
This will direct the conference connection to the MCU room with the ID some_particular_room_id.
### 4.2.2 Connect to an MCU conference to subscribe mix or forward streams {#Conferencesection4_2_2}
Since MCU room can now produce both forward streams and mix stream at the same time, including the screen sharing stream, the client is able to subscribe specified stream(s) by a query string in your URL: mix. The default value for the key word is true.

For example, to subscribe mix stream and screen sharing stream from MCU, connect to the MCU sample application server XXXXX with the following URL:

        https://XXXXX:3004/?mix=true

### 4.2.3 Connect to an MCU conference with screen sharing {#Conferencesection4_2_3}
The client can connect to the MCU conference with screen sharing stream.
To share your screen, use a query string in your URL, via the key word: screen. The default value for the key word is false.
To do this, connect to the MCU sample application server XXXXX with the following URL:

        https://XXXXX:3004/?screen=true


> **Note**:	The screen sharing example in this section requires the Chrome
> extension. Source code of such extension sample is provided in
> Javascript SDK package for reference. Also there are steps in the user guide
> for you to follow from the main page of Intel CS for WebRTC Client SDK for
> JavaScript.
### 4.2.4 Connect to an MCU conference with a specific video resolution {#Conferencesection4_2_4}

In most cases, you can customize the video capture device on the client machine to produce video streams with different resolutions. To specify your local video resolution and send that resolution value to the MCU, use a query string in your URL with the key word "resolution".

The supported video resolution list includes:

        'sif': (320 x 240), 'vga': (640 x 480), 'hd720p': (1280 x 720), 'hd1080p': (1920 x 1080).

For example, if you want to generate a 720P local video stream and publish that to MCU server, you can connect to the MCU sample application server XXXXX with the following URL:

        https://XXXXX:3004/?resolution=hd720p

> **Note**	The specified resolution acts only as a target value. This means that the actual generated video resolution might be different
> depending on the hardware of your local media capture device.
### 4.2.5 Connect to an MCU conference with a RTSP input {#Conferencesection4_2_5}
The MCU conference supports external stream input from devices that support RTSP protocol, like IP Camera.
For example, connect to the MCU sample application server XXXXX with the following URL:

        https://XXXXX:3004/?url=rtsp_stream_url

# 5 Peer Server {#Conferencesection5}
## 5.1 Introduction {#Conferencesection5_1}
The peer server is the default signaling server of the Intel CS for WebRTC. The peer server provides the ability to exchange WebRTC signaling messages over Socket.IO between different clients, as well as provides chat room management.

**Figure 5-1. Peer Server Framework**
<img src="./pic/framework.png" alt="Framework" style="width: 600px;">

## 5.2 Installation requirements {#Conferencesection5_2}
The installation requirements for the peer server are listed in Table 5-1 and 5-2.

**Table 5-1. Installation requirements**
Component name | OS version
----|-----
Peer server | Ubuntu 14.04 LTS, 64-bit

> **Note**: 	The peer server has been fully tested on Ubuntu14.04 LTS,64-bit.
**Table 5-2. Peer Server Dependencies**
Name | Version | Remarks
-----|----|----
Node.js | 4.* | Website: http://nodejs.org/
Node modules | Specified | N/A

Regarding Node.js*, make sure it's installed in your system prior to installing the Peer Server. We recommend version 4.1.2. Refer to http://nodejs.org/ for installation details.
## 5.3 Installation {#Conferencesection5_3}
On the server machine, unpack the peer server release package, and install node modules

        tar –zxvf CS_WebRTC_Conference_Server_Peer.v<Version>.tgz
        cd PeerServer-Release-<Version>
        npm install

The default http port is 8095, and the default secured port is 8096.  Those default values can be modified in the file config.json.
## 5.4 Use your own certificate  {#Conferencesection5_4}
If you want to use a secured socket.io connection, you should use your own certificate for the service. A default certificate is stored in cert with two files: cert.pem and key.pem. Replace these files with your own certificate and key files.
## 5.5 Launch the peer server {#Conferencesection5_5}
Run the following commands to launch the peer server:

        cd PeerServer-Release-<Version>
        node peerserver.js

## 5.6 Stop the peer server {#Conferencesection5_6}
Run the **kill** command directly from the terminal to stop the peer server.
