// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIORANKERWRAPPER_H
#define AUDIORANKERWRAPPER_H

#include <AudioRanker.h>
#include <nan.h>


/*
 * Wrapper class of owt_base::AudioRanker
 */
class AudioRanker : public Nan::ObjectWrap,
                    public owt_base::AudioRanker::Visitor {
public:
    static NAN_MODULE_INIT(Init);
    owt_base::AudioRanker* me;

    boost::mutex mutex;
    std::queue<std::string> jsonChanges;

    void onRankChange(
        std::vector<std::pair<std::string, std::string>> updates) override;

private:
    AudioRanker();
    ~AudioRanker();

    Nan::Callback *callback_;
    uv_async_t async_;
    Nan::AsyncResource *asyncResource_;

    static Nan::Persistent<v8::Function> constructor;

    static NAN_METHOD(New);

    static NAN_METHOD(close);

    static NAN_METHOD(addOutput);

    static NAN_METHOD(addInput);

    static NAN_METHOD(removeInput);

    static NAUV_WORK_CB(Callback);

};

#endif
