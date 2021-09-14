/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportStream.h"
#include "../common/MediaFramePipelineWrapper.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(QuicTransportStream, "QuicTransportStream");

Nan::Persistent<v8::Function> QuicTransportStream::s_constructor;

const int uuidSizeInBytes = 16;
const int frameHeaderSize = 4;

QuicTransportStream::QuicTransportStream()
    : QuicTransportStream(nullptr)
{
}
QuicTransportStream::QuicTransportStream(owt::quic::WebTransportStreamInterface* stream)
    : m_stream(stream)
    , m_contentSessionId(uuidSizeInBytes)
    , m_receivedContentSessionIdSize(0)
    , m_trackId(uuidSizeInBytes)
    , m_receivedTrackIdSize(0)
    , m_readingTrackId(false)
    , m_isPiped(false)
    , m_hasSink(false)
    , m_buffer(nullptr)
    , m_bufferSize(0)
    , m_isMedia(false)
    , m_readingFrameSize(false)
    , m_frameSizeOffset(0)
    , m_frameSizeArray(new uint8_t[frameHeaderSize])
    , m_currentFrameSize(0)
    , m_receivedFrameOffset(0)
{
}

QuicTransportStream::~QuicTransportStream()
{
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnContentSessionId))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnContentSessionId), NULL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnTrackId))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnTrackId), NULL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnData))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnData), NULL);
    }
    m_stream->SetVisitor(nullptr);
    delete[] m_buffer;
    m_onDataCallback.Reset();
}

void QuicTransportStream::OnCanRead()
{
    if (m_receivedContentSessionIdSize < uuidSizeInBytes) {
        ReadContentSessionId();
    } else if (m_readingTrackId) {
        ReadTrackId();
    } else {
        SignalOnData();
    }
}

void QuicTransportStream::OnCanWrite()
{
    ELOG_DEBUG("On can write.");
}

void QuicTransportStream::OnFinRead()
{
    ELOG_DEBUG("On FIN read.");
}

void QuicTransportStream::AddedDestination()
{
    m_hasSink = true;
    if (m_stream->ReadableBytes() > 0) {
        SignalOnData();
    }
}

void QuicTransportStream::RemovedDestination()
{
    // When all destinations are removed, set m_hasSink to false.
}

void QuicTransportStream::addAudioDestination(owt_base::FrameDestination* dest)
{
    owt_base::FrameSource::addAudioDestination(dest);
    AddedDestination();
}

void QuicTransportStream::addVideoDestination(owt_base::FrameDestination* dest)
{
    owt_base::FrameSource::addVideoDestination(dest);
    AddedDestination();
};

void QuicTransportStream::addDataDestination(owt_base::FrameDestination* dest)
{
    owt_base::FrameSource::addDataDestination(dest);
    AddedDestination();
};

NAN_MODULE_INIT(QuicTransportStream::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportStream").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "write", write);
    Nan::SetPrototypeMethod(tpl, "readTrackId", readTrackId);
    Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
    Nan::SetAccessor(instanceTpl, Nan::New("isMedia").ToLocalChecked(), isMediaGetter, isMediaSetter);
    Nan::SetAccessor(instanceTpl, Nan::New("ondata").ToLocalChecked(), onDataGetter, onDataSetter);

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
    uv_async_init(uv_default_loop(), &obj->m_asyncOnContentSessionId, &QuicTransportStream::onContentSessionId);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnTrackId, &QuicTransportStream::onTrackId);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnData, &QuicTransportStream::onData);
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportStream::write)
{
    if (info.Length() < 2) {
        Nan::ThrowTypeError("Data is not provided.");
        return;
    }
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    uint8_t* buffer = (uint8_t*)node::Buffer::Data(info[0]->ToObject());
    unsigned int length = info[1]->Uint32Value();
    auto written = obj->m_stream->Write(buffer, length);
    info.GetReturnValue().Set(Nan::New(static_cast<int>(written)));
}

NAN_METHOD(QuicTransportStream::close)
{
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    obj->m_stream->Close();
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
        isNanDestination = info[2]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked()->Value();
    }
    owt_base::FrameDestination* dest(nullptr);
    if (isNanDestination) {
        NanFrameNode* param = Nan::ObjectWrap::Unwrap<NanFrameNode>(info[1]->ToObject());
        dest = param->FrameDestination();
    } else {
        ::FrameDestination* param = node::ObjectWrap::Unwrap<::FrameDestination>(
            info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
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
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    obj->m_hasSink = false;
}

NAN_METHOD(QuicTransportStream::readTrackId){
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    obj->ReadTrackId();
}

void QuicTransportStream::CheckReadableData()
{
    if (m_stream->ReadableBytes() > 0) {
        SignalOnData();
    }
}

NAN_GETTER(QuicTransportStream::isMediaGetter){
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    info.GetReturnValue().Set(Nan::New(obj->m_isMedia));
}

NAN_SETTER(QuicTransportStream::isMediaSetter)
{
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    obj->m_isMedia = value->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked()->Value();
}

NAN_GETTER(QuicTransportStream::onDataGetter)
{
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    info.GetReturnValue().Set(Nan::New(obj->m_onDataCallback));
}

NAN_SETTER(QuicTransportStream::onDataSetter)
{
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    if (value->IsFunction()) {
        Nan::Persistent<v8::Value> persistentCallback(value);
        obj->m_onDataCallback.Reset(persistentCallback);
        obj->CheckReadableData();
    } else {
        obj->m_onDataCallback.Reset();
    }
}

v8::Local<v8::Object> QuicTransportStream::newInstance(owt::quic::WebTransportStreamInterface* stream)
{
    Local<Object> streamObject = Nan::NewInstance(Nan::New(QuicTransportStream::s_constructor)).ToLocalChecked();
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    obj->m_stream = stream;
    obj->ReadContentSessionId();
    return streamObject;
}

void QuicTransportStream::ReadTrackId()
{
    m_readingTrackId = true;
    size_t readSize = std::min(uuidSizeInBytes - m_receivedTrackIdSize, m_stream->ReadableBytes());
    if (readSize == 0) {
        return;
    }
    m_stream->Read(m_trackId.data() + m_receivedTrackIdSize, readSize);
    m_receivedTrackIdSize += readSize;
    if (m_receivedTrackIdSize == uuidSizeInBytes) {
        m_readingTrackId = false;
        m_asyncOnTrackId.data = this;
        uv_async_send(&m_asyncOnTrackId);
    }
    if (m_stream->ReadableBytes() > 0) {
        OnCanRead();
    }
}

void QuicTransportStream::ReadContentSessionId()
{
    size_t readSize = std::min(uuidSizeInBytes - m_receivedContentSessionIdSize, m_stream->ReadableBytes());
    if (readSize == 0) {
        return;
    }
    m_stream->Read(m_contentSessionId.data() + m_receivedContentSessionIdSize, readSize);
    m_receivedContentSessionIdSize += readSize;
    if (m_receivedContentSessionIdSize == uuidSizeInBytes) {
        m_asyncOnContentSessionId.data = this;
        uv_async_send(&m_asyncOnContentSessionId);
        // Only signaling stream is not piped.
        for (uint8_t d : m_contentSessionId) {
            if (d != 0) {
                m_isPiped = true;
                break;
            }
        }
    }
    if (m_stream->ReadableBytes() > 0) {
        OnCanRead();
    }
}

NAUV_WORK_CB(QuicTransportStream::onData)
{
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (obj == nullptr) {
        return;
    }
    // TODO: Check m_onDataCallback instead of reading ondata.
    Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("ondata").ToLocalChecked());
    if (!onEvent.IsEmpty()) {
        v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
        if (onEventLocal->IsFunction()) {
            v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
            Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("ondata").ToLocalChecked());
            auto readableBytes = obj->m_stream->ReadableBytes();
            uint8_t* buffer = new uint8_t[readableBytes]; // Use a shared buffer instead to reduce performance cost on new.
            obj->m_stream->Read(buffer, readableBytes);
            Local<Value> args[] = { Nan::NewBuffer((char*)buffer, readableBytes).ToLocalChecked() };
            resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
        }
    }
}

NAUV_WORK_CB(QuicTransportStream::onTrackId){
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (obj == nullptr) {
        return;
    }
    Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("ontrackid").ToLocalChecked());
    if (!onEvent.IsEmpty()) {
        v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
        if (onEventLocal->IsFunction()) {
            v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
            Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("ontrackid").ToLocalChecked());
            Local<Value> args[] = { Nan::CopyBuffer((char*)obj->m_trackId.data(), uuidSizeInBytes).ToLocalChecked() };
            resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
        }
    }
}

NAUV_WORK_CB(QuicTransportStream::onContentSessionId)
{
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (obj == nullptr) {
        return;
    }
    Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("oncontentsessionid").ToLocalChecked());
    if (!onEvent.IsEmpty()) {
        v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
        if (onEventLocal->IsFunction()) {
            v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
            Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("oncontentsessionid").ToLocalChecked());
            Local<Value> args[] = { Nan::CopyBuffer((char*)obj->m_contentSessionId.data(), uuidSizeInBytes).ToLocalChecked() };
            resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
        }
    }
}

void QuicTransportStream::SignalOnData()
{
    if (!m_isPiped) {
        m_asyncOnData.data = this;
        uv_async_send(&m_asyncOnData);
        return;
    }

    if (!m_hasSink) {
        return;
    }

    while (m_stream->ReadableBytes() > 0) {
        auto readableBytes = m_stream->ReadableBytes();
        // Future check if it's an audio stream or video stream. Audio is not supported at this time.
        if (m_isMedia) {
            // A new frame.
            if (m_currentFrameSize == 0 && m_receivedFrameOffset == 0 && !m_readingFrameSize) {
                m_readingFrameSize = true;
                memset(m_frameSizeArray, 0, frameHeaderSize * sizeof(uint8_t));
                m_frameSizeOffset = 0;
            }
            // Read frame header.
            if (m_readingFrameSize) {
                size_t readSize = std::min(frameHeaderSize - m_frameSizeOffset, m_stream->ReadableBytes());
                m_stream->Read(m_frameSizeArray + m_frameSizeOffset, readSize);
                m_frameSizeOffset += readSize;
                if (m_frameSizeOffset == frameHeaderSize) {
                    // Usually only the last 2 bytes are used. The first two bits could be used for indicating frame size.
                    for (int i = 0; i < frameHeaderSize; i++) {
                        m_currentFrameSize <<= 8;
                        m_currentFrameSize += m_frameSizeArray[i];
                    }
                    if (m_currentFrameSize > m_bufferSize) {
                        ReallocateBuffer(m_currentFrameSize);
                    }
                    m_readingFrameSize = false;
                }
                continue;
            }
            // Read frame body.
            if (m_receivedFrameOffset < m_currentFrameSize) {
                // Append data to current frame.
                size_t readBytes = std::min(readableBytes, m_currentFrameSize - m_receivedFrameOffset);
                m_stream->Read(m_buffer + m_receivedFrameOffset, readBytes);
                m_receivedFrameOffset += readBytes;
            }
            // Complete frame.
            if (m_receivedFrameOffset == m_currentFrameSize) {
                owt_base::Frame frame;
                frame.format = owt_base::FRAME_FORMAT_I420;
                frame.length = m_currentFrameSize;
                frame.payload = m_buffer;
                // Transport layer doesn't know a frame's type. Video agent is able to parse the type of a frame from bistream. However, video agent doesn't feed the frame to decoder when a key frame is requested.
                frame.additionalInfo.video.isKeyFrame = "key";
                deliverFrame(frame);
                m_currentFrameSize = 0;
                m_receivedFrameOffset = 0;
            }
        } else {
            if (readableBytes > m_bufferSize) {
                ReallocateBuffer(readableBytes);
            }
            owt_base::Frame frame;
            frame.format = owt_base::FRAME_FORMAT_DATA;
            frame.length = readableBytes;
            frame.payload = m_buffer;
            m_stream->Read(frame.payload, readableBytes);
            deliverFrame(frame);
        }
    }
}

void QuicTransportStream::ReallocateBuffer(size_t size)
{
    if (size > m_bufferSize) {
        delete[] m_buffer;
    }
    m_buffer = new uint8_t[size];
    m_bufferSize = size;
    if (!m_buffer) {
        ELOG_ERROR("Failed to allocate buffer.");
    }
}

void QuicTransportStream::onFeedback(const owt_base::FeedbackMsg&)
{
    // No feedbacks righ now.
}

void QuicTransportStream::onFrame(const owt_base::Frame& frame)
{
    m_stream->Write(frame.payload, frame.length);
}

void QuicTransportStream::onVideoSourceChanged()
{
    // Do nothing.
}