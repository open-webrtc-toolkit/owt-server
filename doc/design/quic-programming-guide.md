| OWT QUIC Programming Guide |


# Overview

OWT QUIC SDK and Client SDKs provides the APIs for enabling QUIC Transport for reliable data transmission over QUIC protocol with OWT Conference Server.

# Scope

This document describes the programming models and API interfaces for following usage scenarios.

- Deploying a QUIC conference server that is capable of forwarding data over QUIC channel.
- Implementing client application that is capable of sending data to QUIC server or receiving data from it.

Description of the details of QUIC transport is outside the scope of this document.

# Related Repos

Below are the repo locations of current SDK and server implementations:

- OWT QUIC C++ SDK: [https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-deps-quic](https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-sdk-quic) This is the C++ implementation of Server-side and Client-side SDK, and is the base of enabling QUIC agent on server, and QUIC conference SDK on client.
- OWT Conference server: [https://github.com/open-webrtc-toolkit/owt-server](https://github.com/open-webrtc-toolkit/owt-server/pull/113). This is the server repo that supports forwarding of QUIC streams.
- OWT JavaScript SDK: [https://github.com/open-webrtc-toolkit/owt-client-javascript](https://github.com/open-webrtc-toolkit/owt-client-javascript) This is the repo for enabling QUIC client on browser.
- OWT Native SDK: [https://github.com/open-webrtc-toolkit/owt-client-native](https://github.com/open-webrtc-toolkit/owt-client-native) This is the client SDK repo for enabling QUIC support using OWT conferencing API.

# Architecture

The topology of components is shown in below diagram:

![plot](./pics/quic_block_diagram.png)

There are a few components involved and their relationships with streaming using QuicTransport are described as below:

## OWT QUIC C++ SDK

This is the foundation of QUIC implementation in OWT. It provides the APIs to create QUIC transport clients and server forming a C/S architecture. Basically you can directly build your QUICTransport applications using the QUIC C++ SDK client API if you're not using the OWT native SDK.

## OWT Native SDK for Conference

The OWT conference SDK built upon OWT QUIC C++ SDK client API. It is used in combination with OWT QUIC conference server when you rely on OWT signaling protocol for QUIC connection setup as well as stream forwarding.

## OWT QUIC Conference Server

The OWT QUIC conference server implements the signaling protocol for QUIC connection setup, as well as QUIC stream forwarding.

## OWT QUIC JavaScript SDK

Used together with OWT QUIC conference server, to build Web-based QUIC applications. Useful when the QUIC streaming application is implemented using Web API.

# How to build OWT QUIC C++ SDK

Below are the steps for building the QUIC SDK from [https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-deps-quic](https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-sdk-quic) .

## System Requirements

- At least 50GB of free disk space.
- High speed network connection that is able to
- Windows 10 with Windows SDK 19041 or above, and Visual Studio 2019 for Windows build; or Ubuntu 18.04(LTS) for Ubuntu build.
- Python 3.0 or higher in your path.

## Install Dependencies

Please follow [Chromium Windows build instruction](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/windows_build_instructions.md) or [Chromium Linux build instruction](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux/build_instructions.md) to setup system and install depot\_tools.

## Get the Code

Create a new directory for the check out, and create a .gclient file in this directory. Add following code to .gclient file.
````
solutions = [
  { "name"        : "src/owt",
    "url"         : "https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-deps-quic.git",
    "deps_file"   : "DEPS",
    "managed"     : False,
    "custom_deps" : {
    },
    "custom_vars": {},
  },
]
````
Run `gclient sync` to check out SDK code, Chromium code, and other dependencies. It may take one or two hours if your network connection is not fast enough.

You will get `src` directory after sync completes. Switch to the `src` directory for following steps.

## Build SDK

After sync is done, run `python src/owt/test/scripts/build.py` to build the SDK. Pick up the built binary from src/packages/<commit_id> dir.

## Certificates

Encryption is mandatory for QUIC connections. You may generate a testing certificate by running `net/tools/quic/certs/generate-certs.sh` under chromium source. It is valid for 14 days.

# OWT QUIC C++ SDK API

In this section we provide a detailed description of the APIs provided by OWT QUIC SDK.

## OWT QUIC SDK Server API Calling Flow

The server API calling flow is shown in below diagram and table.

![plot](./pics/server_API.png)

| Step # | API calling flow |
| --- | --- |
| 1 | Application calls the factory method of QuicTransportFactory::Create() to get an instance of QuicTransportFactory |
| 2 | Application calls QuicTransportFactory::CreateQuicTransportServer() on the QuicTransportFactory instance got in step #1, specifying the server port, certificate path either in the form of .pkcs12 or .pfx format. |
| 3 | Application Creates the Visitor instance of QuicTransportServerInterface |
| 4 | Application calls SetVisitor on the QuicTransportServerInterface instance got in step #2. |
| 5 | Application calls Start() method on the QuicTransportServerInterface instance got in step #2 |
| 6 | OnSession() callback will be invoked once the quictransportserver gets a connection request from client, and an QuicTransportSession instance is passed from the callback. |
| 7 | Application calls CreateBidirectionalStream on the QuicTransportSession got in step 6 and get a QuicTransportStreamInterface instance. |
| 8 | Application creates QuicTransportStream::Visitor instance and invokes QuicTransportStreamInterface::SetVisitor(). |
| 9 | Application reads the QuicTransportStream when OnCanRead is invoked on the QuicTransportStream::Visitor; or write to the QuicTransportStream when OnCanWrite is invoked on the QuicTransportStreamVisitor; |

## OWT QUIC C++ SDK Client API Calling Flow

The client API calling flow is shown in below diagram and table. It's similar as the server side calling flow except the QuicTransportFactory creates a QuicTransportClientInterface, instead of a QUICTransportServerInterface, and client needs to call Connect() instead of Start() to get a QuicTransportSession.

![plot](./pics/client_API.png)

| Step # | API calling flow |
| --- | --- |
| 1 | Application calls the factory method of QuicTransportFactory::Create() to get an instance of QuicTransportFactory |
| 2 | Application calls QuicTransportFactory::CreateQuicTransportClient() on the QuicTransportFactory instance got in step #, specifying the server URL to connect to. |
| 3 | Application Creates the Visitor instance of QuicTransportClientInterface |
| 4 | Application calls SetVisitor on the QuicTransportClientInterface instance got in step #2. |
| 5 | Application calls Connect() method on the QuicTransportClientInterface instance got in step #2, passing the URL of the server. |
| 6 | OnSession() callback will be invoked once the quictransportclient successfully connects to server, and an QuicTransportSession instance is passed from the callback. |
| 7 | Application calls CreateBidirectionalStream on the QuicTransportSession got in step 6 and get a QuicTransportStreamInterface instance. |
| 8 | Application creates QuicTransportStream::Visitor instance and invokes QuicTransportStreamInterface::SetVisitor(). |
| 9 | Application reads the QuicTransportStream when OnCanRead is invoked on the QuicTransportStream::Visitor; or write to the QuicTransportStream when OnCanWrite is invoked on the QuicTransportStreamVisitor; |

## Details of Callbacks and Data Structures of QUIC C++ SDK

Please refer to [https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-sdk-quic/-/tree/master/quic\_transport/api/owt/quic/](https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-sdk-quic/-/tree/master/quic_transport/api/owt/quic/) for the detailed API list and document.

## Samples of QUIC C++ SDK

Please refer to the test implementation at [https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-sdk-quic/-/blob/master/quic\_transport/impl/tests/quic\_transport\_owt\_end\_to\_end\_test.cc](https://gitlab.devtools.intel.com/open-webrtc-toolkit/owt-sdk-quic/-/blob/master/quic_transport/impl/tests/quic_transport_owt_end_to_end_test.cc) for how to use the server and client APIs.

# OWT Native Conference SDK

In this section we briefly introduce the build process of OWT Native Conference SDK.

## How to build OWT Native Conference SDK

### Prepare the Development Environment

Before you start, make sure you have the following prerequisites installed/built:

- [WebRTC stack build dependencies](https://webrtc.googlesource.com/src/+/refs/heads/master/docs/native-code/development/prerequisite-sw/index.md).
- [OpenSSL 1.1.0l or higher](https://www.openssl.org/source/).

The following dependencies are for Windows only:

- [Boost 1.67.0 or higher](https://www.boost.org/users/download/).
- [Intel Media SDK for Windows, version 2020 R1 or higher](https://software.intel.com/en-us/media-sdk/choose-download/client).

### Get the Code

- Make sure you checkout the source code to a directory named `src`.
- Create a file named `.gclient` in the directory above the `src` dir, with these contents:

````
solutions = [ 
  {  
     "managed": False,  
     "name": "src",  
     "url": "https://github.com/open-webrtc-toolkit/owt-client-native.git",  
     "custom_deps": {},  
     "deps_file": "DEPS",  
     "safesync_url": "",  
  },  
]  
target_os = []  
````

### Build

#### Windows

- Set the environment variable BOOST\_ROOT to your boost source tree.
- Run `gclient sync`. It may take a long time to download a large amount of data.
- Go to `src` dir, and run `gn args out/release-x64`, in the prompted file, input:

```
is_clang = false
enable_libaom = true
rtc_use_h264 = true
rtc_use_h265 = true
is_component_build = false
use_lld = false
rtc_build_examples = false
treat_warnings_as_errors = false
target_cpu = "x64"
is_debug = false
ffmpeg_branding = "Chrome"
rtc_include_tests = false
owt_include_tests = false
owt_use_quic = true
owt_quic_header_root = "d:\\workspace\\quic\\include"; #example
owt_quic_lib_root = "d:\\workspace\\quic\\bin\\Release" #example
````

Please be noted the owt\_quic\_header\_root points to the "include" dir the package built from C++ QUIC SDK, and the owt\_quic\_lib\_root points to the bin/release/ or bin/debug directory that contains owt\_quic\_transport.lib|dll|so.

Still in `src` dir, run `ninja -C out/release-x64`. The built binary will be under `out/release-x64/obj/talk/owt/owt.lib`.

#### Linux

You would need Ubuntu 18.04 to build the client SDK. **CentOS is currently not supported**.

- First make sure you have correct proxy set for apt and also for https\_proxy/http\_proxy set for shell if you're behind firewall.
- Make sure you have python2.7 installed and it's the first python in the path to be used in shell.
- Run gclient sync. It make take a long time to download a large amount of data. The sync may fail due to some of the dependencies missing. In that case, run src/build/install-build-deps.sh to install the biuld dependencies for Linux.
- After gclient sync finishes successfully, in `src` dir, run `python scripts/build_linux.py --gn_gen --sdk --use_gcc –arch x64 –scheme release –quic_root=<path_to_quic_sdk_binary_containing_include_and_lib_dir>`

The output library will be under `src/out/release-x64/obj/talk/owt/libowt.a`, and header files that can be used by sample will be under `src/talk/owt/sdk/include/cpp`.

# OWT Native Conferene SDK API

OWT conference SDK provides a series of API for wrapping the underlying QUIC SDK to communicate with the OWT conference server.

Typical calling flow for publishing QUIC stream:

![plot](./pics/publishing_flow.png)

Typical calling flow for Subscribing QUIC stream:

![plot](./pics/subscribing_flow.png)

Please see the conference sample application for more detailed usage.

# OWT QUIC Conference Server

In this section we brief introduce the OWT QUIC Conference Server Build steps.

## How to install development dependencies

In the repository root, use one of following commands to install the dependencies.

- Interactive mode: scripts/installDeps.sh
- Non-interactive mode: scripts/installDepsUnattended.sh In interactive mode, you need type &quot;yes&quot; to continue installation several times and in non-interactive, the installation continues automatically.

## How to Build OWT Conference Server 

### Requirements

The OWT QUIC conference server can be built on the following platforms:

- Ubuntu 18.04
- CentOS 7.6

### Build Instructions

In the root of the repository:

1. Build native components: scripts/build.js -t all --check.
2. Pack built components and js files: scripts/pack.js -t all --install-module --app-path ${webrtc-javascript-sdk-sample-conference-dist}.

The ${webrtc-javascript-sdk-sample-conference-dist} is built from owt-javascript-sdk, e.g. ~/owt-client-javascript/dist/sample/conference, see [https://github.com/open-webrtc-toolkit/owt-client-javascript](https://github.com/open-webrtc-toolkit/owt-client-javascript) for details.

If &quot;--archive ${name}&quot; option is appended to the pack command, a &quot;Release-${name}.tgz&quot; file will be generated in root folder. For other options, run the scripts with &quot;--help&quot; option.

## How to use Pre-built Conference Server Binary

Steps to run Conference Server with pre-built binary:

1. Go to the untarred conference server dir, and run `./bin/init-all.sh --deps`; this would try to install mongodb and rabbitmq-server. Don't set any password for rabbitmq or mongodb;
2. Still in the same dir, run `bin/init-all.sh`; you will get output like:

````
superServiceId: '5f5aeca7228ecf057595d1dd'
superServiceKey: 'wovlSvy0O94DmtIqBsRIbsd4OnmGURNxXRkN1B2IYj3/Cz+iXMualCEjbNPMo7h/hitv+1vwyiV9TksEAQCFuf2OUIkJRuu6ihwUAj5X+bo6GQe3Ar0ufvf2mQBg77armaofvaORS4n9iKpcUFdA8+YTN/lri5yyp+pCleql96o='
sampleServiceId: '5f5aeca7228ecf057595d1de'
sampleServiceKey: 'veLzLLAEId7a7Sz1ayoJSMcY5SjxW4WxJqmfeDxgbespBZ80Z4n95TwlGqSaydTVHl4FQGI4rkEFdGeU9ENKXSZPUahTVwHCmWl37Pj9aCiO9HQCJOzHMH5scYnkZ28KDduWLzCoTKJQ6F48ctwosMAGPO4L0H7wXdy2ASAf/ZU='
````

3. Modify app/current\_app/samplesrtcservice.js, update the line with the sampleServiceId and sampleServiceKey above in step 2.
````
icsREST.API.init('5f5aeca7228ecf057595d1de', 
'veLzLLAEId7a7Sz1ayoJSMcY5SjxW4WxJqmfeDxgbespBZ80Z4n95TwlGqSaydTVHl4FQGI4rkEFdGeU9ENKXSZPUahTVwHCmWl37Pj9aCiO9HQCJOzHMH5scYnkZ28KDduWLzCoTKJQ6F48ctwosMAGPO4L0H7wXdy2ASAf/ZU=',
'https://localhost:3000/', false);
````

4. run `bin/start-all.sh`.
5. Open [https://localhost:3004/quic.html](https://localhost:3004/quic.html) with Chrome browser to visit the web sample page. Due to the test certificate, you may need confirm this unsafe access.
6. Press 'Start Sending' button to start transferring data to conference server. Press 'Stop Sending' button on the web page to stop sending.. If there's no error in the console, that implies server is correctly setup.

# OWT QUIC Windows Sample

The windows sample will be provided in OWT repo separately. More details will be provided later.

# How to Replace the Certificate for QUIC

OWT Conference Server is using a self-signed certificate during development phase, which would be only valid for 14 days. You can use a CA-signed certificate to avoid refreshing the certificate periodically. QUIC connection will fail if certificate is not valid or expires.

## Precondition

- Make sure you are running the tool under Linux and,
- Openssl tool is correctly setup in your system.
- Download the tool under chromium/src/net/tools/quic/certs/ from chromium project. This contains three files: ca.cnf, generate-certs.sh and leaf.cnf.

## Certificate Generation

- Modify leaf.cnf, adding an entry into "other_hosts" section.
- Make sure generate-certs.sh is exectuable. If not, run `chmod +x generate-certs.sh`;
- Remove the out dir in case it exists.
- Under the certtools dir, run `./generate-certs.sh`. It is expected to generate a series of files under out dir.
- Under the certtools dir, run `openssl pkcs12 -inkey out/leaf\_certs.key -in out/leaft\_cert.pem -export -out out/certificate.pfx`. This will prompt for password for the pfx. Make sure you always use abc123 as the password.
- Under the certtools dir, run `openssl x509 -noout -fingerprint -sha256 -inform pem -in out/leaf\_cert.pem`. You will get the fingerprint string in the form of "XX:XX:XX....XX:XX".

## Use the Certificate

- Copy the generated certificate.pfx under out dir to quic\_agent/cert/ dir to replace the one there.
- Restart Conference Server QUIC agent to apply the change. If you're using JS sample for QUIC, make sure you also update JS sample with the new fingerprint.
- In your native client sample, make sure you include the fingerprint of new cert in the ConferenceClientConfiguration.trusted\_quic\_certificate\_fingerprints you passed to ConferenceClient's ctor. See more details in the conference sample.

