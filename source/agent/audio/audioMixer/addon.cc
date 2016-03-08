#include "AudioMixerWrapper.h"

#include <node.h>

using namespace v8;

NODE_MODULE(addon, AudioMixer::Init)
