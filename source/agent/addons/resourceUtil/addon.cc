#include "ResourceUtilWrapper.h"

#include <node.h>

using namespace v8;

// void InitAll(Handle<Object> exports) {
//   PipelineWrapper::Init(exports);
// }

NODE_MODULE(addon, ResourceUtil::Init)
