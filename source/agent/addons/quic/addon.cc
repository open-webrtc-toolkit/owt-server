
#include "RTCIceTransport.h"
#include "RTCIceCandidate.h"
#include "RTCQuicTransport.h"
#include "RTCQuicStream.h"
#include "QuicTransportServer.h"
#include "../../webrtc/webrtcLib/ThreadPool.h"
#include "../../webrtc/webrtcLib/IOThreadPool.h"
#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports)
{
    ThreadPool::Init(exports);
    IOThreadPool::Init(exports);
    RTCQuicTransport::Init(exports);
    RTCIceTransport::Init(exports);
    RTCIceCandidate::Init(exports);
    //RTCCertificate::Init(exports);
    RTCQuicStream::Init(exports);
    QuicTransportServer::Init(exports);
}

NODE_MODULE(addon, InitAll)