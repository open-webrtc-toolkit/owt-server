// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportClient.h"
/*#include <thread>
#include <chrono>*/
#include <iostream>

using namespace net;
//using namespace owt_base;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;
const size_t INIT_BUFF_SIZE = 80000;

Nan::Persistent<v8::Function> QuicTransportClient::s_constructor;

DEFINE_LOGGER(QuicTransportClient, "QuicTransportClient");

// QUIC Outgoing
QuicTransportClient::QuicTransportClient(const std::string& dest_ip, int dest_port)
        : client_(RQuicFactory::createQuicClient())
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0) {
    m_receiveData.buffer.reset(new char[m_bufferSize]);
    printf("QuicOut:QiucOut\n");
    client_->setListener(this);
    client_->start(dest_ip.c_str(), dest_port);
}

QuicTransportClient::~QuicTransportClient() {
    client_->stop();
    client_.reset();
}

NAN_MODULE_INIT(QuicTransportClient::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportClient").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "send", send);
    Nan::SetPrototypeMethod(tpl, "onNewStreamEvent", onNewStreamEvent);
    Nan::SetPrototypeMethod(tpl, "onRoomEvent", onRoomEvent);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportClient").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportClient::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() < 2) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    
    Nan::Utf8String paramId(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string host = std::string(*paramId);
    int port = Nan::To<int32_t>(info[1]).FromJust();
    QuicTransportClient* obj = new QuicTransportClient(host, port);
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnNewStream, &QuicTransportClient::onNewStreamCallback);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnRoomEvents, &QuicTransportClient::onRoomCallback);
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportClient::onNewStreamEvent) {
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());

  obj->has_stream_callback_ = true;
  obj->stream_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(QuicTransportClient::onRoomEvent) {
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());

  obj->has_room_callback_ = true;
  obj->room_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(QuicTransportClient::send) {
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());
  
  unsigned int session_id = Nan::To<int>(info[0]).FromJust();
  unsigned int stream_id = Nan::To<int>(info[1]).FromJust();
  Nan::Utf8String param1(Nan::To<v8::String>(info[2]).ToLocalChecked());
  std::string data = std::string(*param1);
  
  obj->sendStream(session_id, stream_id, data);
}

NAUV_WORK_CB(QuicTransportClient::onNewStreamCallback){
    Nan::HandleScope scope;
    QuicTransportClient* obj = reinterpret_cast<QuicTransportClient*>(async->data);
    if (!obj) {
        return;
    }

    boost::mutex::scoped_lock lock(obj->mutex);
    if (obj->has_stream_callback_) {
      while (!obj->stream_messages.empty()) {
          Local<Value> args[] = {Nan::New(obj->stream_messages.front().c_str()).ToLocalChecked(),
              Nan::New(obj->stream_messages.front().c_str()).ToLocalChecked()};
          Nan::AsyncResource resource("streamCallback");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->stream_callback_->GetFunction(), 2, args);
          obj->stream_messages.pop();
      }
    }
}

NAUV_WORK_CB(QuicTransportClient::onRoomCallback){
    Nan::HandleScope scope;
    QuicTransportClient* obj = reinterpret_cast<QuicTransportClient*>(async->data);
    if (!obj) {
        return;
    }

    boost::mutex::scoped_lock lock(obj->mutex);
    if (obj->has_room_callback_) {
      while (!obj->room_messages.empty()) {
          Local<Value> args[] = {Nan::New(obj->room_messages.front().c_str()).ToLocalChecked(),
              Nan::New(obj->room_messages.front().c_str()).ToLocalChecked()};
          Nan::AsyncResource resource("roomCallback");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->room_callback_->GetFunction(), 2, args);
          obj->room_messages.pop();
      }
    }
}

void QuicTransportClient::sendStream(uint32_t session_id, uint32_t stream_id, const std::string& data) {
    std::cout << "client send data: " << data << "with size:" << data.length() << std::endl;
    TransportData sendData;
    uint32_t payloadLength = data.length();
    sendData.buffer.reset(new char[payloadLength + 4]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength);
    memcpy(sendData.buffer.get() + 4, data.c_str(), payloadLength);
    sendData.length = payloadLength + 4;

    //std::string str(data.buffer.get(), data.length);
    if (sendData.length > INIT_BUFF_SIZE + 4) {
        std::cout << "sendFrame " << (sendData.length  - 4)<< std::endl;
    }
    printf("data address in addon is:%s\n", data.c_str());
    client_->send(session_id, stream_id, sendData.buffer.get(), sendData.length);
}


void QuicTransportClient::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    std::cout << "client onData: " << buf << " len:" << len << " m_bufferSize:" << m_bufferSize << " m_receivedBytes:" << m_receivedBytes << std::endl;

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
    std::cout << "Copy data and m_receivedBytes:" << m_receivedBytes << std::endl;
    while (m_receivedBytes >= 4) {
        std::cout << "m_receivedBytes >=4";
        uint32_t payloadlen = 0;
        payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer.get())));
        uint32_t expectedLen = payloadlen + 4;
        std::cout << "m_receivedBytes:" << m_receivedBytes << " payloadlen:" << payloadlen << " expectedLen:" << expectedLen << std::endl;
        if (expectedLen > m_receivedBytes) {
            // continue
            std::cout << "expectedLen:" << expectedLen << " m_receivedBytes:" << m_receivedBytes << std::endl;
            break;
        } else {
            // std::cout << "receive: " << expectedLen << std::endl;
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;
            std::string s_data(dpos, payloadlen);
            std::cout << "notify roomevents:" << s_data << std::endl;

            /*std::string str;
            str.append("{\"sessionId\":\"");
            str.append(std::to_string(session_id));
            str.append("\",\"streamId\":\"");
            str.append(std::to_string(stream_id));
            str.append("\",\"data\":\"");
            str.append(s_data.c_str());
            str.append("\"}");*/

            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }

            boost::mutex::scoped_lock lock(mutex);
            this->room_messages.push(s_data);
            m_asyncOnRoomEvents.data = this;
            uv_async_send(&m_asyncOnRoomEvents);
        }
    }
}

void QuicTransportClient::onReady(uint32_t session_id, uint32_t stream_id) {
    std::string data("{\"sessionId\":");
    data.append(std::to_string(session_id));
    data.append(", \"streamId\":");
    data.append(std::to_string(stream_id));
    data.append("}");
    printf("Quic out ready with data:%s\n", data.c_str());

    boost::mutex::scoped_lock lock(mutex);
    this->stream_messages.push(data);
    m_asyncOnNewStream.data = this;
    uv_async_send(&m_asyncOnNewStream);
}

