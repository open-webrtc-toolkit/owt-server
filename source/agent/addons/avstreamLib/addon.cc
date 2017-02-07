#include "AVStreamInWrap.h"
#include "AVStreamOutWrap.h"
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

void SetAVLogLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    if (!args[0]->IsNumber()) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        return;
    }
    int level = args[0]->IntegerValue();
    switch (level) {
    case AV_LOG_QUIET: /* -8 */
    case AV_LOG_PANIC: /*  0 */
    case AV_LOG_FATAL: /*  8 */
    case AV_LOG_ERROR: /* 16 */
    case AV_LOG_WARNING: /* 24 */
    case AV_LOG_INFO: /* 32 */
    case AV_LOG_VERBOSE: /* 40 */
    case AV_LOG_DEBUG: /* 48 */
    case AV_LOG_TRACE: /* 56 */
        av_log_set_level(level);
        args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
        return;
    default:
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
    }
}

void InitAll(Handle<Object> exports)
{
    auto isolate = exports->GetIsolate();
    AVStreamInWrap::Init(exports);
    AVStreamOutWrap::Init(exports);
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_QUIET"), v8::Integer::New(isolate, AV_LOG_QUIET));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_PANIC"), v8::Integer::New(isolate, AV_LOG_PANIC));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_FATAL"), v8::Integer::New(isolate, AV_LOG_FATAL));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_ERROR"), v8::Integer::New(isolate, AV_LOG_ERROR));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_WARNING"), v8::Integer::New(isolate, AV_LOG_WARNING));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_INFO"), v8::Integer::New(isolate, AV_LOG_INFO));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_VERBOSE"), v8::Integer::New(isolate, AV_LOG_VERBOSE));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_DEBUG"), v8::Integer::New(isolate, AV_LOG_DEBUG));
    exports->Set(v8::String::NewFromUtf8(isolate, "AV_LOG_TRACE"), v8::Integer::New(isolate, AV_LOG_TRACE));
    NODE_SET_METHOD(exports, "setAVLogLevel", SetAVLogLevel);

    // initialize ffmeg/libav
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_WARNING);
}

NODE_MODULE(addon, InitAll)
