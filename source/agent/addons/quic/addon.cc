#include "QuicTransportServer.h"
#include <node.h>

using namespace v8;

NAN_MODULE_INIT(InitAll)
{
    QuicTransportServer::init(target);
    QuicTransportConnection::init(target);
    QuicTransportStream::init(target);
}

NODE_MODULE(addon, InitAll)
