#include "QuicTransportServer.h"
#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports)
{
    QuicTransportServer::Init(exports);
}

NODE_MODULE(addon, InitAll)
