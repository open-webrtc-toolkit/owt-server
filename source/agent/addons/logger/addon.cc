
#include <nan.h>
// include log4cxx header files
#include <log4cxx/propertyconfigurator.h>

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;
using Nan::GetFunction;
using Nan::New;
using Nan::Set;

// Configure using the "log4cxx.properties" file
NAN_METHOD(configure) {
  // expect a number as the first argument
  log4cxx::PropertyConfigurator::configure("log4cxx.properties");
}

// Expose synchronous and asynchronous access to our
// Estimate() function
NAN_MODULE_INIT(InitAll) {
  Set(target, New<String>("configure").ToLocalChecked(),
    GetFunction(New<FunctionTemplate>(configure)).ToLocalChecked());
}

NODE_MODULE(addon, InitAll)