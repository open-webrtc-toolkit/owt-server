// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "Utils.h"
#include <thread>
#include <chrono>
#include <iostream>
#include "QuicTransportStream.h"

//using namespace net;
using namespace owt_base;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;
const char TDT_MEDIA_METADATA = 0x3A;
const size_t INIT_BUFF_SIZE = 80000;

DEFINE_LOGGER(QuicTransportStream, "QuicTransportStream");

Nan::Persistent<v8::Function> QuicTransportStream::s_constructor;

static void dump(void* index, uint8_t* buf, int len)
{
    char dumpFileName[128];

    snprintf(dumpFileName, 128, "/tmp/quicstream-%p", index);
    FILE* bsDumpfp = fopen(dumpFileName, "ab");
    if (bsDumpfp) {
        fwrite(buf, 1, len, bsDumpfp);
        fclose(bsDumpfp);
    }
}

// QUIC Incomming
QuicTransportStream::QuicTransportStream()
    : QuicTransportStream(nullptr)
{
}

QuicTransportStream::QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream)
        : m_stream(stream)
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0)
        , m_needKeyFrame(true)
        , m_trackKind("unknown") {
    m_receiveData.buffer.reset(new char[m_bufferSize]);
}

QuicTransportStream::~QuicTransportStream() {
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnData))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnData), NULL);
    }
    m_stream->SetVisitor(nullptr);
    /*delete[] m_receiveData.buffer;
    data_callback_.Reset();*/
}

NAN_MODULE_INIT(QuicTransportStream::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportStream").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
    Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);
    Nan::SetPrototypeMethod(tpl, "send", send);
    Nan::SetPrototypeMethod(tpl, "onStreamData", onStreamData);
    Nan::SetPrototypeMethod(tpl, "getId", getId);
    Nan::SetAccessor(instanceTpl, Nan::New("trackKind").ToLocalChecked(), trackKindGetter, trackKindSetter);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportStream").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportStream::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    QuicTransportStream* obj = new QuicTransportStream();
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnData, &QuicTransportStream::onStreamDataCallback);
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> QuicTransportStream::newInstance(owt::quic::QuicTransportStreamInterface* stream)
{
    ELOG_DEBUG("QuicTransportStream::newInstance");
    Local<Object> streamObject = Nan::NewInstance(Nan::New(QuicTransportStream::s_constructor)).ToLocalChecked();
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    obj->m_stream = stream;
    return streamObject;
}

NAN_METHOD(QuicTransportStream::send) {
  ELOG_DEBUG("QuicTransportStream::send");
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
  
  Nan::Utf8String param1(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string data = std::string(*param1);
  
  obj->sendData(data);
}

NAN_METHOD(QuicTransportStream::addDestination)
{
    ELOG_DEBUG("QuicTransportStream::addDestination");
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    if (info.Length() > 3) {
        Nan::ThrowTypeError("Invalid argument length for addDestination.");
        return;
    }
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string track = std::string(*param0);
    bool isNanDestination(false);
    if (info.Length() == 3) {
        ELOG_DEBUG("addDestination has 3 parameters");
        isNanDestination = Nan::To<bool>(info[2]).FromJust();
    }
    owt_base::FrameDestination* dest(nullptr);
    if (isNanDestination) {
        ELOG_DEBUG("addDestination isNanDestination is true");
        NanFrameNode* param = Nan::ObjectWrap::Unwrap<NanFrameNode>(
            Nan::To<v8::Object>(info[1]).ToLocalChecked());
        dest = param->FrameDestination();
    } else {
        ELOG_DEBUG("addDestination isNanDestination is false");
        ::FrameDestination* param = node::ObjectWrap::Unwrap<::FrameDestination>(
            Nan::To<v8::Object>(info[1]).ToLocalChecked());
        dest = param->dest;
    }

    if (track == "audio") {
        obj->addAudioDestination(dest);
    } else if (track == "video") {
        obj->addVideoDestination(dest);
    } else if (track == "data") {
        obj->addDataDestination(dest);
    }
}

NAN_METHOD(QuicTransportStream::removeDestination)
{
    ELOG_DEBUG("QuicTransportStream::removeDestination");
    //QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
}

NAN_METHOD(QuicTransportStream::onStreamData) {
  ELOG_DEBUG("QuicTransportStream::onStreamData");
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());

  obj->has_data_callback_ = true;
  obj->data_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAUV_WORK_CB(QuicTransportStream::onStreamDataCallback){
    ELOG_DEBUG("QuicTransportStream::onStreamDataCallback");
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (!obj) {
        return;
    }

    //boost::mutex::scoped_lock lock(obj->mutex);

    if (obj->has_data_callback_) {
      while (!obj->data_messages.empty()) {
          ELOG_DEBUG("data_messages is not empty");
          obj->m_dataQueueMutex.lock();
          Local<Value> args[] = { Nan::New(obj->data_messages.front().c_str()).ToLocalChecked() };
          obj->m_dataQueueMutex.unlock();
          Nan::AsyncResource resource("onStreamData");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->data_callback_->GetFunction(), 1, args);
          obj->data_messages.pop();
      }
    }
    ELOG_DEBUG("QuicTransportStream::onStreamDataCallback ends");
}

NAN_METHOD(QuicTransportStream::getId) {
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
  ELOG_DEBUG("QuicTransportStream::getId:%d\n", obj->m_stream->Id());
  info.GetReturnValue().Set(Nan::New(obj->m_stream->Id()));
}

NAN_GETTER(QuicTransportStream::trackKindGetter){
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    info.GetReturnValue().Set(Nan::New(obj->m_trackKind).ToLocalChecked());
}

NAN_SETTER(QuicTransportStream::trackKindSetter) {
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
  Nan::Utf8String trackKind(Nan::To<v8::String>(value).ToLocalChecked());
  obj->m_trackKind = std::string(*trackKind);
  ELOG_DEBUG("QuicTransportStream::setTrackKind with value:%s", obj->m_trackKind.c_str());
}

void QuicTransportStream::onFeedback(const FeedbackMsg& msg) {
    ELOG_DEBUG("QuicTransportStream::onFeedback");
    TransportData sendData;
    uint32_t payloadLength = sizeof(FeedbackMsg);
    sendData.buffer.reset(new char[payloadLength + 5]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength + 1);
    sendData.buffer[4] = TDT_FEEDBACK_MSG;
    memcpy(sendData.buffer.get() + 5, reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), payloadLength);
    sendData.length = payloadLength + 5;

    m_stream->SendData(sendData.buffer.get(), sendData.length);
}

void QuicTransportStream::onVideoSourceChanged()
{
    ELOG_DEBUG("QuicTransportStream::onVideoSourceChanged");
    // Do nothing.
}

void QuicTransportStream::onFrame(const owt_base::Frame& frame)
{
    //ELOG_DEBUG("QuicTransportStream::onFrame");
    dump(this, frame.payload, frame.length);
    TransportData sendData;
    sendData.buffer.reset(new char[sizeof(Frame) + frame.length + 5]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(sizeof(Frame) + frame.length + 1);
    sendData.buffer[4] = TDT_MEDIA_FRAME;
    memcpy(sendData.buffer.get() + 5, reinterpret_cast<uint8_t*>(const_cast<Frame*>(&frame)),
           sizeof(Frame));
    memcpy(sendData.buffer.get() + 5 + sizeof(Frame), frame.payload, frame.length);
    sendData.length = sizeof(Frame) + frame.length + 5;

    m_stream->SendData(sendData.buffer.get(), sendData.length);
}


void QuicTransportStream::sendData(const std::string& data) {
    ELOG_DEBUG("QuicTransportStream::sendData:%s\n", data.c_str());
    TransportData sendData;
    uint32_t payloadLength = data.length();
    sendData.buffer.reset(new char[payloadLength + 5]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength + 1);
    sendData.buffer[4] = TDT_MEDIA_METADATA;
    memcpy(sendData.buffer.get() + 5, data.c_str(), payloadLength);
    sendData.length = payloadLength + 5;

    m_stream->SendData(sendData.buffer.get(), sendData.length);
}

void QuicTransportStream::sendFeedback(const FeedbackMsg& msg) {
    TransportData sendData;
    uint32_t payloadLength = sizeof(FeedbackMsg);
    sendData.buffer.reset(new char[payloadLength + 5]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength + 1);
    sendData.buffer[4] = TDT_FEEDBACK_MSG;
    memcpy(sendData.buffer.get() + 5, reinterpret_cast<uint8_t*>(const_cast<FeedbackMsg*>(&msg)),
           sizeof(FeedbackMsg));
    sendData.length = payloadLength + 5;
    ELOG_DEBUG("QuicTransportStream::sendFeedback:%s\n", sendData.buffer.get());

    m_stream->SendData(sendData.buffer.get(), sendData.length);
}

void QuicTransportStream::OnData(owt::quic::QuicTransportStreamInterface* stream, char* buf, size_t len) {
    if (m_receivedBytes + len >= m_bufferSize) {
        m_bufferSize += (m_receivedBytes + len);
        std::cout << "new_bufferSize: " << m_bufferSize << std::endl;
        boost::shared_array<char> new_buffer;
        new_buffer.reset(new char[m_bufferSize]);
        if (new_buffer.get()) {
            memcpy(new_buffer.get(), m_receiveData.buffer.get(), m_receivedBytes);
            m_receiveData.buffer.reset();
            m_receiveData.buffer = new_buffer;
        } else {
            return;
        }
    }
    memcpy(m_receiveData.buffer.get() + m_receivedBytes, buf, len);
    m_receivedBytes += len;
    while (m_receivedBytes >= 4) {
        uint32_t payloadlen = 0;

        payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer.get())));
        uint32_t expectedLen = payloadlen + 4;
        if (expectedLen > m_receivedBytes) {
            // continue
            break;
        } else {
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;
            Frame* frame = nullptr;
            std::string s_data(dpos + 1, payloadlen - 1);
            owt_base::FeedbackMsg msg {.type = owt_base::VIDEO_FEEDBACK, .cmd = owt_base::REQUEST_KEY_FRAME};

            switch (dpos[0]) {
                case TDT_MEDIA_FRAME:
                    //ELOG_DEBUG("QuicTransportStream deliver frame with trackKind: %s", m_trackKind.c_str());
                    frame = reinterpret_cast<Frame*>(dpos + 1);
                    frame->payload = reinterpret_cast<uint8_t*>(dpos + 1 + sizeof(Frame));
                    if (m_trackKind == "video") {
                      if (m_needKeyFrame) {
                        if (frame->additionalInfo.video.isKeyFrame) {
                            m_needKeyFrame = false;
                        } else {
                            ELOG_DEBUG("Request key frame\n");
                            sendFeedback(msg);
                            return;
                        }
                      }
                    }
                    dump(this, frame->payload, frame->length);
                    deliverFrame(*frame);
                    break;
                case TDT_MEDIA_METADATA:
                    ELOG_DEBUG("QuicTransportStream::onData with type TDT_MEDIA_METADATA%s", s_data.c_str());
                    m_dataQueueMutex.lock();
                    this->data_messages.push(s_data);
                    m_dataQueueMutex.unlock();
                    m_asyncOnData.data = this;
                    uv_async_send(&m_asyncOnData);
                    ELOG_DEBUG("QuicTransportStream::onData with type TDT_MEDIA_METADATA end");
                    break;
                case TDT_FEEDBACK_MSG:
                    ELOG_DEBUG("QuicTransportStream deliver feedback msg");
                    deliverFeedbackMsg(msg);
                    break;
                default:
                    break;
            }

            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }
        }
    }
}
