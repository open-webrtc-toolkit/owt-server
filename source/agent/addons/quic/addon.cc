
#include "RTCIceTransport.h"
#include "RTCIceCandidate.h"
#include "RTCQuicTransport.h"
//#include "RTCCertificate.h"
//#include "QuicStream.h"
#include "../../webrtc/webrtcLib/ThreadPool.h"
#include "../../webrtc/webrtcLib/IOThreadPool.h"
#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  ThreadPool::Init(exports);
  IOThreadPool::Init(exports);
  RTCQuicTransport::Init(exports);
  RTCIceTransport::Init(exports);
  RTCIceCandidate::Init(exports);
  //RTCCertificate::Init(exports);
  //QuicStream::Init(exports);
}

NODE_MODULE(addon, InitAll)
