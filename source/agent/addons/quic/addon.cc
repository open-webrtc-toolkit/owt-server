#include "QuicTransportServer.h"
#include <node.h>

using namespace v8;

NAN_MODULE_INIT(InitAll)
{
    QuicTransportServer::Init(target);
}

NODE_MODULE(addon, InitAll)
