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
Gateway Server | CentOS* 7.1

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
<td><ul><li>Google Chrome* v49</li>
<li>Firefox* v45</li></ul>
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
To install gateway server, do as following:

        tar xf CS_WebRTC_Gateway_SIP.<Version>.tgz
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
