/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "WebTransportFrameDestination.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(WebTransportFrameDestination, "WebTransportFrameDestination");

Nan::Persistent<v8::Function> WebTransportFrameDestination::s_constructor;

const std::string audioTrackId = "00000000000000000000000000000001";
const std::string videoTrackId = "00000000000000000000000000000002";

WebTransportFrameDestination::WebTransportFrameDestination(const std::string& subscriptionId, bool isDatagram)
    : m_isDatagram(isDatagram)
    , m_datagramOutput(nullptr)
    , m_rtpFactory(nullptr)
    , m_videoRtpPacketizer(nullptr)
{
#ifdef OWT_FAKE_RTP
    m_rtpFactory = m_rtpFactory->createFakeFactory();
#else
    m_rtpFactory = m_rtpFactory->createDefaultFactory();
#endif
}

NAN_MODULE_INIT(WebTransportFrameDestination::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("WebTransportFrameDestination").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "addDatagramOutput", addDatagramOutput);
    Nan::SetPrototypeMethod(tpl, "removeDatagramOutput", removeDatagramOutput);
    Nan::SetPrototypeMethod(tpl, "addStreamOutput", addStreamOutput);
    Nan::SetPrototypeMethod(tpl, "removeStreamOutput", removeStreamOutput);
    Nan::SetPrototypeMethod(tpl, "receiver", receiver);
    Nan::SetAccessor(instanceTpl, Nan::New("rtpConfig").ToLocalChecked(), rtpConfigGetter);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("WebTransportFrameDestination").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(WebTransportFrameDestination::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() < 2) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string subscriptionId = std::string(*param0);
    bool isDatagram = Nan::To<bool>(info[1]).FromJust();
    WebTransportFrameDestination* obj = new WebTransportFrameDestination(subscriptionId, isDatagram);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(WebTransportFrameDestination::addDatagramOutput)
{
    WebTransportFrameDestination* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameDestination>(info.Holder());
    std::unique_lock<std::shared_timed_mutex> lock(obj->m_datagramOutputMutex);
    if (obj->m_datagramOutput) {
        ELOG_WARN("Datagram output exists, will be replaced by the new one.");
        obj->m_datagramOutput->FrameDestination()->setDataSource(nullptr);
    }
    if (!obj->m_videoRtpPacketizer) {
        obj->m_videoRtpPacketizer = obj->m_rtpFactory->createVideoPacketizer();
        obj->m_videoRtpPacketizer->addDataDestination(obj);
    }
    NanFrameNode* output = Nan::ObjectWrap::Unwrap<NanFrameNode>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
    obj->m_datagramOutput = output;
    // TODO: Use addDataDestination defined in MediaFramePipeline. We use m_datagramOutput here because the output could also be streams.
    output->FrameDestination()->setDataSource(obj);
}

NAN_METHOD(WebTransportFrameDestination::removeDatagramOutput)
{
    WebTransportFrameDestination* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameDestination>(info.Holder());
    std::unique_lock<std::shared_timed_mutex> lock(obj->m_datagramOutputMutex);
    if (!obj->m_datagramOutput) {
        ELOG_WARN("Datagram output does not exist.");
        return;
    }
    obj->m_videoRtpPacketizer.reset();
    obj->m_datagramOutput->FrameDestination()->setDataSource(nullptr);
    obj->m_datagramOutput = nullptr;
}

NAN_METHOD(WebTransportFrameDestination::addStreamOutput)
{
    if (info.Length() < 2) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    WebTransportFrameDestination* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameDestination>(info.Holder());
    std::unique_lock<std::shared_timed_mutex> lock(obj->m_streamOutputMutex);
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string track = std::string(*param0);
    NanFrameNode* output = Nan::ObjectWrap::Unwrap<NanFrameNode>(Nan::To<v8::Object>(info[1]).ToLocalChecked());
    if (obj->m_streamOutput.find(track) != obj->m_streamOutput.end()) {
        ELOG_WARN("Stream output for track %s exists, will be replaced by the new one.", track);
        obj->m_streamOutput[track]->FrameDestination()->setDataSource(nullptr);
    }
    // TODO: Use addDataDestination defined in MediaFramePipeline. We use m_streamOutput here because the output could also be datagrams.
    obj->m_streamOutput[track] = output;
    output->FrameDestination()->setDataSource(obj);
    ELOG_DEBUG("Add stream output.");
}

NAN_METHOD(WebTransportFrameDestination::removeStreamOutput)
{
    if (info.Length() < 1) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    WebTransportFrameDestination* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameDestination>(info.Holder());
    std::unique_lock<std::shared_timed_mutex> lock(obj->m_streamOutputMutex);
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string track = std::string(*param0);
    if (obj->m_streamOutput.find(track) == obj->m_streamOutput.end()) {
        ELOG_WARN("Stream output for track %s does not exist.", track);
        return;
    }
    obj->m_streamOutput[track]->FrameDestination()->setDataSource(nullptr);
    obj->m_streamOutput.erase(track);
    ELOG_DEBUG("Remove stream output.");
}

NAN_METHOD(WebTransportFrameDestination::receiver)
{
    info.GetReturnValue().Set(info.This());
}

NAN_GETTER(WebTransportFrameDestination::rtpConfigGetter)
{
    WebTransportFrameDestination* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameDestination>(info.Holder());
    if (!obj->m_videoRtpPacketizer) {
        info.GetReturnValue().Set(Nan::Undefined());
        return;
    }
    RtpConfig video = obj->m_videoRtpPacketizer->getRtpConfig();
    v8::Local<v8::Object> rtpConfig = Nan::New<v8::Object>();
    v8::Local<v8::Object> videoConfig = Nan::New<v8::Object>();
    Nan::Set(videoConfig, Nan::New("ssrc").ToLocalChecked(), Nan::New<v8::Number>(video.ssrc));
    Nan::Set(rtpConfig, Nan::New("video").ToLocalChecked(), videoConfig);
    info.GetReturnValue().Set(rtpConfig);
}

void WebTransportFrameDestination::onFrame(const owt_base::Frame& frame)
{
    // Packetize video frames or send RTP packets.
    if (frame.format != owt_base::FRAME_FORMAT_RTP) {
        if (m_isDatagram && m_videoRtpPacketizer) {
            m_videoRtpPacketizer->onFrame(frame);
        } else if (!m_isDatagram) {
            DispatchMediaFrame(frame);
        }
    } else {
        std::shared_lock<std::shared_timed_mutex> lock(m_datagramOutputMutex, std::defer_lock);
        if (!m_datagramOutput) {
            ELOG_DEBUG("Datagram output is not ready, dropping frame.");
            return;
        }
        m_datagramOutput->FrameDestination()->onFrame(frame);
    }
}

void WebTransportFrameDestination::onVideoSourceChanged() { }

void WebTransportFrameDestination::onFeedback(const owt_base::FeedbackMsg& feedback)
{
    // TODO: RTCP packet could be for audio. Sending to audio packetizer when audio support is added.
    if (!m_videoRtpPacketizer) {
        ELOG_WARN("RTP packetizer is not available.");
        return;
    }
    m_videoRtpPacketizer->onFeedback(feedback);
}

void WebTransportFrameDestination::DispatchMediaFrame(const owt_base::Frame& frame)
{
    std::shared_lock<std::shared_timed_mutex> lock(m_streamOutputMutex);
    owt_base::FrameDestination* dest(nullptr);
    if (owt_base::isAudioFrame(frame)) {
        if (m_streamOutput.find(audioTrackId) == m_streamOutput.end()) {
            ELOG_DEBUG("Audio output not found. Dropping an audio frame.");
            return;
        }
        dest = m_streamOutput[audioTrackId]->FrameDestination();
    } else if (owt_base::isVideoFrame(frame)) {
        if (m_streamOutput.find(videoTrackId) == m_streamOutput.end()) {
            ELOG_DEBUG("Video output not found. Dropping a video frame.");
            return;
        }
        dest = m_streamOutput[videoTrackId]->FrameDestination();
    } else {
        ELOG_ERROR("Unknown frame type.");
        return;
    }
    // Write header. 4 bytes for the size of the body.
    uint32_t payloadSize(frame.length);
    uint8_t* buffer = new uint8_t[4];
    for (int i = 0; i < 4; i++) {
        buffer[3 - i] = payloadSize & 0xFF;
        payloadSize >>= 8;
    }
    owt_base::Frame header;
    header.format = owt_base::FRAME_FORMAT_DATA;
    header.length = 4;
    header.payload = buffer;
    dest->onFrame(header);
    // Write body.
    dest->onFrame(frame);
}