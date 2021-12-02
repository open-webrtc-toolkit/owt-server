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

// QUIC Incomming
QuicTransportStream::QuicTransportStream()
    : QuicTransportStream(nullptr)
{
}

QuicTransportStream::QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream)
        : m_stream(stream)
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0) {
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
    Nan::SetPrototypeMethod(tpl, "getId", getId);

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
    Local<Object> streamObject = Nan::NewInstance(Nan::New(QuicTransportStream::s_constructor)).ToLocalChecked();
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    obj->m_stream = stream;
    return streamObject;
}

NAN_METHOD(QuicTransportStream::send) {
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
  
  Nan::Utf8String param1(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string data = std::string(*param1);
  
  obj->sendData(data);
}

NAN_METHOD(QuicTransportStream::addDestination)
{
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    if (info.Length() > 3) {
        Nan::ThrowTypeError("Invalid argument length for addDestination.");
        return;
    }
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string track = std::string(*param0);
    bool isNanDestination(false);
    if (info.Length() == 3) {
        isNanDestination = Nan::To<bool>(info[2]).FromJust();
    }
    owt_base::FrameDestination* dest(nullptr);
    if (isNanDestination) {
        NanFrameNode* param = Nan::ObjectWrap::Unwrap<NanFrameNode>(
            Nan::To<v8::Object>(info[1]).ToLocalChecked());
        dest = param->FrameDestination();
    } else {
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
    //QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
}

NAN_METHOD(QuicTransportStream::onStreamData) {
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());

  obj->has_data_callback_ = true;
  obj->data_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAUV_WORK_CB(QuicTransportStream::onStreamDataCallback){
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (!obj) {
        return;
    }

    boost::mutex::scoped_lock lock(obj->mutex);

    if (obj->has_data_callback_) {
      while (!obj->data_messages.empty()) {
          Local<Value> args[] = { Nan::New(obj->data_messages.front().c_str()).ToLocalChecked() };
          Nan::AsyncResource resource("sessionCallback");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->data_callback_->GetFunction(), 1, args);
          obj->data_messages.pop();
      }
    }
}

NAN_METHOD(QuicTransportStream::getId) {
  QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());

  info.GetReturnValue().Set(Nan::New(obj->m_stream->Id()));
}

void QuicTransportStream::onFeedback(const FeedbackMsg& msg) {
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
    // Do nothing.
}

void QuicTransportStream::onFrame(const owt_base::Frame& frame)
{
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
    TransportData sendData;
    uint32_t payloadLength = data.length();
    sendData.buffer.reset(new char[payloadLength + 5]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength + 1);
    sendData.buffer[4] = TDT_MEDIA_METADATA;
    memcpy(sendData.buffer.get() + 5, data.c_str(), payloadLength);
    sendData.length = payloadLength + 5;

    m_stream->SendData(sendData.buffer.get(), sendData.length);
}

void QuicTransportStream::OnData(owt::quic::QuicTransportStreamInterface* stream, char* buf, size_t len) {
    std::cout << "server onData: " << buf << " len:" << len << " m_bufferSize:" << m_bufferSize << " m_receivedBytes:" << m_receivedBytes << std::endl;

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
	std::cout << "m_receivedBytes:" << m_receivedBytes << " payloadlen:" << payloadlen << " expectedLen:" << expectedLen << std::endl;
        if (expectedLen > m_receivedBytes) {
            // continue
            break;
        } else {
            std::cout << "receive: " << expectedLen << std::endl;
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;
            Frame* frame = nullptr;
            std::string s_data(dpos, payloadlen);

            switch (dpos[0]) {
                case TDT_MEDIA_FRAME:
                    frame = reinterpret_cast<Frame*>(dpos + 1);
                    frame->payload = reinterpret_cast<uint8_t*>(dpos + 1 + sizeof(Frame));
                    deliverFrame(*frame);
                    break;
                case TDT_MEDIA_METADATA:
                    this->data_messages.push(s_data);
                    m_asyncOnData.data = this;
                    uv_async_send(&m_asyncOnData);
                    break;
                case TDT_FEEDBACK_MSG:
                    deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(dpos + 1)));
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
