// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportServer.h"
#include "Utils.h"
#include <thread>
#include <chrono>
#include <iostream>

using namespace net;
using namespace owt_base;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;
const size_t INIT_BUFF_SIZE = 80000;

DEFINE_LOGGER(QuicTransportServer, "QuicTransportServer");

Nan::Persistent<v8::Function> QuicTransportServer::s_constructor;

// QUIC Incomming
QuicTransportServer::QuicTransportServer(unsigned int port, const std::string& cert_file, const std::string& key_file)
        : m_quicServer(RQuicFactory::createQuicServer(cert_file.c_str(), key_file.c_str()))
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0) {
  m_receiveData.buffer.reset(new char[m_bufferSize]);
  m_quicServer->setListener(this);
  m_quicServer->listen(port);
}

QuicTransportServer::~QuicTransportServer() {
    m_quicServer->stop();
    m_quicServer.reset();
}

NAN_MODULE_INIT(QuicTransportServer::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportServer").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "getListeningPort", getListeningPort);
/*    Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
    Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);*/
    Nan::SetPrototypeMethod(tpl, "send", send);
    Nan::SetPrototypeMethod(tpl, "onNewStreamEvent", onNewStreamEvent);
    Nan::SetPrototypeMethod(tpl, "onRoomEvent", onRoomEvent);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportServer").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportServer::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() < 3) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    int port = Nan::To<int32_t>(info[0]).FromJust();
    Nan::Utf8String pfxPath(Nan::To<v8::String>(info[1]).ToLocalChecked());
    Nan::Utf8String password(Nan::To<v8::String>(info[2]).ToLocalChecked());
    QuicTransportServer* obj = new QuicTransportServer(port, *pfxPath, *password);
    //owt_base::Utils::ZeroMemory(*password, password.length());
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnNewStream, &QuicTransportServer::onNewStreamCallback);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnRoomEvents, &QuicTransportServer::onRoomCallback);
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportServer::onNewStreamEvent) {
  QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());

  obj->has_stream_callback_ = true;
  obj->stream_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(QuicTransportServer::onRoomEvent) {
  QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());

  obj->has_room_callback_ = true;
  obj->room_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(QuicTransportServer::getListeningPort) {
  QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());


  uint32_t port = obj->getServerPort();

  info.GetReturnValue().Set(Nan::New(port));
}

NAN_METHOD(QuicTransportServer::send) {
  QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());
  
  unsigned int session_id = Nan::To<int>(info[0]).FromJust();
  unsigned int stream_id = Nan::To<int>(info[1]).FromJust();
  Nan::Utf8String param1(Nan::To<v8::String>(info[2]).ToLocalChecked());
  std::string data = std::string(*param1);
  
  obj->sendData(session_id, stream_id, data);
}

/*void QuicIn::onFeedback(const FeedbackMsg& msg) {
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    server_->send((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}*/

NAUV_WORK_CB(QuicTransportServer::onNewStreamCallback){
    Nan::HandleScope scope;
    QuicTransportServer* obj = reinterpret_cast<QuicTransportServer*>(async->data);
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

NAUV_WORK_CB(QuicTransportServer::onRoomCallback){
    Nan::HandleScope scope;
    QuicTransportServer* obj = reinterpret_cast<QuicTransportServer*>(async->data);
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

unsigned int QuicTransportServer::getServerPort() {
    unsigned int port = m_quicServer->getServerPort();
    while (port == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        port = m_quicServer->getServerPort();
    }

    return port;
}


void QuicTransportServer::sendData(uint32_t session_id, uint32_t stream_id, const std::string& data) {
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
    printf("data address in addon is:%s, payloadlength:%d, datalen:%d\n", data.c_str(), payloadLength, sendData.length);
    m_quicServer->send(session_id, stream_id, sendData.buffer.get(), sendData.length);
}

void QuicTransportServer::onReady(uint32_t session_id, uint32_t stream_id) {
    std::cout << "server onReady session id:" << session_id << " stream id:" << stream_id << std::endl;
}

void QuicTransportServer::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    std::cout << "server onData: " << buf << " len:" << len << " m_bufferSize:" << m_bufferSize << " m_receivedBytes:" << m_receivedBytes << std::endl;
    std::string sessionId = std::to_string(session_id);
    std::string streamId = std::to_string(stream_id);
    std::string sId = sessionId + streamId;

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
            std::string s_data(dpos, payloadlen);

            if (hasStream_.count(sId) == 0) {
                hasStream_[sId] = true;
                std::string str;
                str.append("{\"sessionId\":\"");
                str.append(std::to_string(session_id));
                str.append("\",\"streamId\":\"");
                str.append(std::to_string(stream_id));
                str.append("\",\"room\":\"");
                str.append(s_data.c_str());
                str.append("\"}");

                boost::mutex::scoped_lock lock(mutex);
                this->stream_messages.push(str);
                m_asyncOnNewStream.data = this;
                uv_async_send(&m_asyncOnNewStream);

            } else {
                std::string str;
                str.append("{\"sessionId\":\"");
                str.append(std::to_string(session_id));
                str.append("\",\"streamId\":\"");
                str.append(std::to_string(stream_id));
                str.append("\",\"data\":");
                str.append(s_data.c_str());
                str.append("}");

                boost::mutex::scoped_lock lock(mutex);
                this->room_messages.push(str);
                m_asyncOnRoomEvents.data = this;
                uv_async_send(&m_asyncOnRoomEvents);
            }

            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }
        }
    }
}
