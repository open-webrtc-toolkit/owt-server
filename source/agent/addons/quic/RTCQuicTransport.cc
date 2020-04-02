/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTCQuicTransport.h"
#include "IceConnectionAdapter.h"
#include "QuicFactory.h"
#include "RTCQuicStream.h"
#include "owt/quic/quic_definitions.h"
#include "owt/quic/quic_transport_factory.h"

using v8::Array;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::Value;

Nan::Persistent<Function> RTCQuicTransport::s_constructor;

DEFINE_LOGGER(RTCQuicTransport, "RTCQuicTransport");

// Length of HKDF input keying material, equal to its number of bytes.
// https://tools.ietf.org/html/rfc5869#section-2.2.
const size_t kInputKeyingMaterialLength = 32;

RTCQuicTransport::RTCQuicTransport()
{
    ELOG_DEBUG("RTCQuicTransport ctor.");
}

RTCQuicTransport::~RTCQuicTransport()
{
}

NAN_MODULE_INIT(RTCQuicTransport::Init)
{
    //base::TaskScheduler::CreateAndStartWithDefaultParams("RTCQuicTransport");

    ELOG_DEBUG("Init RTCQuicTransport.");
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCQuicTransport").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    //Nan::SetPrototypeMethod(tpl, "getCertificates", getCertificates);
    Nan::SetPrototypeMethod(tpl, "start", start);
    Nan::SetPrototypeMethod(tpl, "getLocalParameters", getLocalParameters);
    Nan::SetPrototypeMethod(tpl, "listen", listen);
    Nan::SetPrototypeMethod(tpl, "createBidirectionalStream", createBidirectionalStream);
    Nan::SetPrototypeMethod(tpl, "stop", stop);

    s_constructor.Reset(tpl->GetFunction());
    Nan::Set(target, Nan::New("RTCQuicTransport").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(RTCQuicTransport::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() == 0) {
        return Nan::ThrowTypeError("RTCIceTransport is required.");
    }
    ELOG_DEBUG("New RTCQuicTransport.");
    RTCQuicTransport* obj = new RTCQuicTransport();
    Nan::MaybeLocal<Value> maybeIceTransport = info[0];
    if (!maybeIceTransport.ToLocalChecked()->IsObject()) {
        return Nan::ThrowTypeError("RTCIceTransport is required.");
    }
    auto rtcIceTransport = Nan::ObjectWrap::Unwrap<RTCIceTransport>(maybeIceTransport.ToLocalChecked()->ToObject());
    erizo::IceConnection* iceConnection = rtcIceTransport->iceConnection();
    ELOG_DEBUG("Got erizo::IceConnection.");
    auto iceAdapter = std::make_unique<IceConnectionAdapter>(IceConnectionAdapter(iceConnection));
    // TODO: Remove listener in dtor.
    rtcIceTransport->setQuicListener(iceAdapter.get());
    obj->m_packetTransport = std::move(iceAdapter);
    auto factory = QuicFactory::getQuicTransportFactory();
    ELOG_DEBUG("Got Quic factory.");
    obj->m_transport = factory->CreateP2PServerTransport(obj->m_packetTransport.get());
    obj->m_transport->SetDelegate(obj);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnStream, &RTCQuicTransport::onStreamCallback);
    /*
    ELOG_DEBUG("Checking certs.");
    std::vector<rtc::scoped_refptr<rtc::RTCCertificate>> certificates;
    Nan::MaybeLocal<Value> maybeCertArray = info[1];
    if (!maybeCertArray.ToLocalChecked()->IsUndefined()) {
        if (!maybeCertArray.ToLocalChecked()->IsArray()) {
            return Nan::ThrowTypeError("certificates is not an array.");
        }
        Local<v8::Array> certArray = Local<v8::Array>::Cast(info[1]);
        ELOG_DEBUG("Get cert arrray.");
        for (unsigned int i = 0; i < certArray->Length(); i++) {
            if (Nan::Has(certArray, i).FromJust()) {
                RTCCertificate* cert = Nan::ObjectWrap::Unwrap<RTCCertificate>(Nan::Get(certArray, i).ToLocalChecked()->ToObject());
                ELOG_DEBUG("Check cert has expired.");
                uint64_t now = rtc::TimeMillis();
                if (cert->certificate()->HasExpired(now)) {
                    return Nan::ThrowError("InvalidAccessError: certificate has expired.");
                }
                ELOG_DEBUG("After checking cert expire.");
                certificates.push_back(cert->certificate());
            }
        }
    } else {
        rtc::scoped_refptr<rtc::RTCCertificate> cert = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams::ECDSA(),
            absl::nullopt);
        ELOG_DEBUG("Generated cert.");
        certificates.push_back(cert);
    }*/
    ELOG_DEBUG("Before obj wrap.");
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    ELOG_DEBUG("Set return value.");
}

NAN_METHOD(RTCQuicTransport::start)
{
    ELOG_DEBUG("RTCQuicTransport::start");
    // RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    // Nan::MaybeLocal<Value> maybeQuicParameters = info[0];
    // if (!maybeQuicParameters.ToLocalChecked()->IsObject()) {
    //     return Nan::ThrowTypeError("Incorrect remote Parameters.");
    // }
    // Local<v8::Object> quicParametersObject = maybeQuicParameters.ToLocalChecked()->ToObject();
    // std::unique_ptr<RTCQuicParameters> remoteParameters = std::make_unique<RTCQuicParameters>();
    // // role.
    // ELOG_DEBUG("Role.");
    // Nan::Utf8String role(Nan::Get(quicParametersObject, Nan::New("role").ToLocalChecked()).ToLocalChecked()->ToString());
    // remoteParameters->role = std::string(*role);
    // // fingerprints.
    // ELOG_DEBUG("Fingerprint.");
    // Local<Value> fingerprintsValue = Nan::Get(quicParametersObject, Nan::New("fingerprints").ToLocalChecked()).ToLocalChecked();
    // if (!fingerprintsValue->IsArray()) {
    //     return Nan::ThrowTypeError("Fingerprints should be an array.");
    // }
    // Local<Array> fingerprints = Local<Array>::Cast(fingerprintsValue);
    // for (unsigned int i = 0; i < fingerprints->Length(); i++) {
    //     auto fingerprint = Nan::Get(fingerprints, i).ToLocalChecked()->ToObject();
    //     Nan::Utf8String algorithm(Nan::Get(fingerprint, Nan::New("algorithm").ToLocalChecked()).ToLocalChecked());
    //     Nan::Utf8String value(Nan::Get(fingerprint, Nan::New("value").ToLocalChecked()).ToLocalChecked());
    //     RTCDtlsFingerprint f;
    //     f.algorithm = std::string(*algorithm);
    //     f.value = std::string(*value);
    //     remoteParameters->fingerprints.push_back(f);
    // }
    // ELOG_DEBUG("Transport->start.");
    // obj->m_transport->start(std::move(remoteParameters));
}

NAN_METHOD(RTCQuicTransport::listen)
{
    ELOG_DEBUG("RTCQuicTransport::listen");
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    char* keyBuffer = node::Buffer::Data(info[0]);
    size_t keyLength = node::Buffer::Length(info[0]);
    const std::string keyString = std::string(keyBuffer, keyLength);
    ELOG_DEBUG("KeyString length is %d", keyString.size());
    //obj->m_transport->Listen(keyString);
    obj->m_transport->Listen((uint8_t*)keyBuffer, keyLength);
    ELOG_DEBUG("KeyString length is %d", keyString.size());
    ELOG_DEBUG("After create P2P Quic transport.");
}

NAN_METHOD(RTCQuicTransport::getLocalParameters)
{
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    owt::quic::RTCQuicParameters local_parameters = obj->m_transport->GetLocalParameters();
    Local<Object> parameters = Nan::New<Object>();
    Nan::Set(parameters, Nan::New("role").ToLocalChecked(), Nan::New(local_parameters.role).ToLocalChecked());
    info.GetReturnValue().Set(parameters);
}

NAN_METHOD(RTCQuicTransport::createBidirectionalStream)
{
    //RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    //quic::QuartcStream* quicStream=obj->m_transport->CreateOutgoingBidirectionalStream();
    //info.GetReturnValue().Set(QuicStream::newInstance(quicStream));
}

NAN_METHOD(RTCQuicTransport::stop)
{
    //RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    ELOG_DEBUG("Stop an RTCQuicTransport.");
    // if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->m_asyncOnStream))) {
    //     uv_close(reinterpret_cast<uv_handle_t*>(&obj->m_asyncOnStream), NULL);
    // }
}

NAUV_WORK_CB(RTCQuicTransport::onStreamCallback)
{
    ELOG_DEBUG("RTCQuicTransport::onStreamCallback")
    Nan::HandleScope scope;
    RTCQuicTransport* obj = reinterpret_cast<RTCQuicTransport*>(async->data);
    if (!obj || obj->m_unnotifiedStreams.empty())
        return;
    std::lock_guard<std::mutex> lock(obj->m_onStreamMutex);
    while (!obj->m_unnotifiedStreams.empty()) {
        v8::Local<v8::Object> stream = RTCQuicStream::newInstance(obj->m_unnotifiedStreams.front());
        Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("onbidirectionalstream").ToLocalChecked());
        if (!onEvent.IsEmpty()) {
            ELOG_DEBUG("onEvent is not empty.")
            v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
            if (onEventLocal->IsFunction()) {
                ELOG_DEBUG("onEventLocal is function.");
                v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
                Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("EventCallback").ToLocalChecked());
                Local<Value> args[] = { stream };
                resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
            }
        } else {
            ELOG_DEBUG("onEvent is empty");
        }
        obj->m_unnotifiedStreams.pop();
    }
}

void RTCQuicTransport::OnIncomingStream(owt::quic::P2PQuicStreamInterface* stream)
{
    assert(stream);
    ELOG_DEBUG("OnIncomingStream.");
    {
        std::lock_guard<std::mutex> lock(m_onStreamMutex);
        m_unnotifiedStreams.push(stream);
    }
    m_asyncOnStream.data = this;
    uv_async_send(&m_asyncOnStream);
}