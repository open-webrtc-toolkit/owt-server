#include "AVStreamInWrap.h"
#include "AVStreamOutWrap.h"
#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports)
{
    AVStreamInWrap::Init(exports);
    AVStreamOutWrap::Init(exports);
}

NODE_MODULE(addon, InitAll)
