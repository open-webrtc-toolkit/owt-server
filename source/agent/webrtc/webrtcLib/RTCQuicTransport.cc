/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTCQuicTransport.h"
#include "P2PQuicTransport.h"
#include "QuicConnectionAdapter.h"
#include "QuicStream.h"
#include "base/task/post_task.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/quic/quic_chromium_alarm_factory.h"
#include "net/third_party/quiche/src/quic/core/tls_server_handshaker.h"
#include "net/third_party/quiche/src/quic/quartc/quartc_connection_helper.h"
#include "net/third_party/quiche/src/quic/quartc/quartc_crypto_helpers.h"
#include "third_party/webrtc/rtc_base/rtc_certificate_generator.h"
#include "third_party/webrtc/rtc_base/ssl_certificate.h"
#include "third_party/webrtc/rtc_base/time_utils.h"
#include "net/quic/platform/impl/quic_chromium_clock.h"

using v8::Array;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::Value;
using namespace logging;

Nan::Persistent<Function> RTCQuicTransport::s_constructor;

DEFINE_LOGGER(RTCQuicTransport, "RTCQuicTransport");

// Length of HKDF input keying material, equal to its number of bytes.
// https://tools.ietf.org/html/rfc5869#section-2.2.
const size_t kInputKeyingMaterialLength = 32;

RTCQuicTransport::RTCQuicTransport()
{
    ELOG_DEBUG("RTCQuicTransport ctor.");
    m_helper = std::make_shared<quic::QuartcConnectionHelper>(quic::QuicChromiumClock::GetInstance());
    m_taskRunner = base::CreateSequencedTaskRunnerWithTraits({ base::MayBlock() });
    m_alarmFactory = std::make_shared<net::QuicChromiumAlarmFactory>(m_taskRunner.get(), quic::QuicChromiumClock::GetInstance());
    m_compressedCertsCache = std::make_shared<quic::QuicCompressedCertsCache>(quic::QuicCompressedCertsCache::kQuicCompressedCertsCacheSize);
    ELOG_DEBUG("End of RTCQuicTransport ctor.");
}
RTCQuicTransport::~RTCQuicTransport()
{
}

NAN_MODULE_INIT(RTCQuicTransport::Init)
{
    base::TaskScheduler::CreateAndStartWithDefaultParams("RTCQuicTransport");

    ELOG_DEBUG("Init RTCQuicTransport.");
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCQuicTransport").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "getCertificates", getCertificates);
    Nan::SetPrototypeMethod(tpl, "start", start);
    Nan::SetPrototypeMethod(tpl, "getLocalParameters", getLocalParameters);
    Nan::SetPrototypeMethod(tpl, "listen", listen);
    Nan::SetPrototypeMethod(tpl, "createBidirectionalStream", createBidirectionalStream);
    Nan::SetPrototypeMethod(tpl, "stop", stop);

    s_constructor.Reset(tpl->GetFunction());
    Nan::Set(target, Nan::New("RTCQuicTransport").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());

    base::CommandLine::Init(0, nullptr);
    // Logging settings for Chromium.
    logging::SetMinLogLevel(LOG_VERBOSE);
    LoggingSettings settings;
    settings.logging_dest = LOG_TO_SYSTEM_DEBUG_LOG;
    InitLogging(settings);
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
    uv_async_init(uv_default_loop(), &obj->m_asyncOnStream, &RTCQuicTransport::onStreamCallback);
    ELOG_DEBUG("Create P2PQuicTransport.");
    auto rtcIceTransport = Nan::ObjectWrap::Unwrap<RTCIceTransport>(maybeIceTransport.ToLocalChecked()->ToObject());
    obj->m_iceTransport = rtcIceTransport;
    //obj->m_transport = obj->createP2PQuicTransport(rtcIceTransport, obj->createServerCryptoConfig());
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

NAN_METHOD(RTCQuicTransport::getCertificates)
{
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    auto rtcCertificates = obj->m_transport->getCertificates();
    Local<v8::Array> certificates = Nan::New<v8::Array>(rtcCertificates.size());
    ELOG_DEBUG("Certificate size: %d.", certificates->Length());
    for (unsigned int i = 0; i < rtcCertificates.size(); i++) {
        Nan::Set(certificates, i, RTCCertificate::newInstance(rtcCertificates[i]));
    }
    info.GetReturnValue().Set(certificates);
}

NAN_METHOD(RTCQuicTransport::start)
{
    ELOG_DEBUG("RTCQuicTransport::start");
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    Nan::MaybeLocal<Value> maybeQuicParameters = info[0];
    if (!maybeQuicParameters.ToLocalChecked()->IsObject()) {
        return Nan::ThrowTypeError("Incorrect remote Parameters.");
    }
    Local<v8::Object> quicParametersObject = maybeQuicParameters.ToLocalChecked()->ToObject();
    std::unique_ptr<RTCQuicParameters> remoteParameters = std::make_unique<RTCQuicParameters>();
    // role.
    ELOG_DEBUG("Role.");
    Nan::Utf8String role(Nan::Get(quicParametersObject, Nan::New("role").ToLocalChecked()).ToLocalChecked()->ToString());
    remoteParameters->role = std::string(*role);
    // fingerprints.
    ELOG_DEBUG("Fingerprint.");
    Local<Value> fingerprintsValue = Nan::Get(quicParametersObject, Nan::New("fingerprints").ToLocalChecked()).ToLocalChecked();
    if (!fingerprintsValue->IsArray()) {
        return Nan::ThrowTypeError("Fingerprints should be an array.");
    }
    Local<Array> fingerprints = Local<Array>::Cast(fingerprintsValue);
    for (unsigned int i = 0; i < fingerprints->Length(); i++) {
        auto fingerprint = Nan::Get(fingerprints, i).ToLocalChecked()->ToObject();
        Nan::Utf8String algorithm(Nan::Get(fingerprint, Nan::New("algorithm").ToLocalChecked()).ToLocalChecked());
        Nan::Utf8String value(Nan::Get(fingerprint, Nan::New("value").ToLocalChecked()).ToLocalChecked());
        RTCDtlsFingerprint f;
        f.algorithm = std::string(*algorithm);
        f.value = std::string(*value);
        remoteParameters->fingerprints.push_back(f);
    }
    ELOG_DEBUG("Transport->start.");
    obj->m_transport->start(std::move(remoteParameters));
}

NAN_METHOD(RTCQuicTransport::listen)
{
    ELOG_DEBUG("RTCQuicTransport::listen");
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    char* keyBuffer = node::Buffer::Data(info[0]);
    size_t keyLength = node::Buffer::Length(info[0]);
    auto cryptoServerConfig = obj->createServerCryptoConfig();
    std::string keyString = std::string(keyBuffer,keyLength);
    cryptoServerConfig->set_pre_shared_key(keyString);
    obj->m_transport = obj->createP2PQuicTransport(obj->m_iceTransport, cryptoServerConfig);
    obj->m_transport->start(nullptr);
    ELOG_DEBUG("After create P2P Quic transport.");
}

NAN_METHOD(RTCQuicTransport::getLocalParameters)
{
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    v8::Local<v8::Array> fingerprints = Nan::New<v8::Array>();
    int fingerprintIndex = 0;
    auto certificates = obj->m_transport->getCertificates();
    for (const auto& certificate : certificates) {
        auto stats = certificate->ssl_certificate().GetStats();
        Local<Object> fingerprint = Nan::New<Object>();
        Nan::Set(fingerprint, Nan::New("alghrithm").ToLocalChecked(), Nan::New(stats->fingerprint_algorithm).ToLocalChecked());
        Nan::Set(fingerprint, Nan::New("value").ToLocalChecked(), Nan::New(stats->fingerprint).ToLocalChecked());
        Nan::Set(fingerprints, fingerprintIndex++, fingerprint);
    }
    Local<Object> parameters = Nan::New<Object>();
    Nan::Set(parameters, Nan::New("fingerprints").ToLocalChecked(), fingerprints);
    Nan::Set(parameters, Nan::New("role").ToLocalChecked(), Nan::New("auto").ToLocalChecked());
    info.GetReturnValue().Set(parameters);
}

NAN_METHOD(RTCQuicTransport::createBidirectionalStream)
{
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    quic::QuartcStream* quicStream=obj->m_transport->CreateOutgoingBidirectionalStream();
    //info.GetReturnValue().Set(QuicStream::newInstance(quicStream));
}

NAN_METHOD(RTCQuicTransport::stop)
{
    RTCQuicTransport* obj = Nan::ObjectWrap::Unwrap<RTCQuicTransport>(info.Holder());
    ELOG_DEBUG("Stop an RTCQuicTransport.");
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->m_asyncOnStream))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&obj->m_asyncOnStream), NULL);
    }
}

NAUV_WORK_CB(RTCQuicTransport::onStreamCallback)
{
    ELOG_DEBUG("RTCQuicTransport::onStreamCallback")
    Nan::HandleScope scope;
    RTCQuicTransport* obj = reinterpret_cast<RTCQuicTransport*>(async->data);
    if (!obj || obj->m_streamsToBeNotified.empty())
        return;
    std::lock_guard<std::mutex> lock(obj->m_streamQueueMutex);
    while (!obj->m_streamsToBeNotified.empty()) {
        ELOG_DEBUG("RTCQuicTransport::onStreamCallback1");
        v8::Local<v8::Object> stream = QuicStream::newInstance(obj->m_streamsToBeNotified.front());
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
        obj->m_streamsToBeNotified.pop();
    }
}

std::shared_ptr<quic::QuicCryptoServerConfig> RTCQuicTransport::createServerCryptoConfig()
{
    // Copied from P2PQuicCryptoConfigFactoryImpl::CreateServerCryptoConfig().
    // Generate a random source address token secret every time since this is
    // a transient client.
    char source_address_token_secret[kInputKeyingMaterialLength];
    quic::QuicRandom::GetInstance()->RandBytes(source_address_token_secret,
        kInputKeyingMaterialLength);
    std::unique_ptr<quic::ProofSource> proof_source(new quic::DummyProofSource);
    ELOG_DEBUG("Created proof source.");
    std::shared_ptr<quic::QuicCryptoServerConfig> config = std::make_shared<quic::QuicCryptoServerConfig>(
        std::string(source_address_token_secret, kInputKeyingMaterialLength),
        quic::QuicRandom::GetInstance(), std::move(proof_source),
        quic::KeyExchangeSource::Default(),
        quic::TlsServerHandshaker::CreateSslCtx());
    config->set_validate_source_address_token(false);
    config->set_chlo_multiplier(1500);
    config->set_validate_chlo_size(false);
    quic::QuicCryptoServerConfig::ConfigOptions options;
    std::unique_ptr<quic::CryptoHandshakeMessage> message(
        config->AddDefaultConfig(
            m_helper->GetRandomGenerator(), m_helper->GetClock(), options));
    config->set_pad_rej(false);
    config->set_pad_shlo(false);
    return config;
}

std::unique_ptr<P2PQuicTransport> RTCQuicTransport::createP2PQuicTransport(RTCIceTransport* iceTransport, std::shared_ptr<quic::QuicCryptoServerConfig> serverCryptoConfig)
{
    ELOG_DEBUG("Create P2PQuicPacketTransportIceAdapter.");
    //P2PQuicPacketTransportIceAdapter* adapter=new P2PQuicPacketTransportIceAdapter(iceTransport->iceConnection());
    m_quicPacketTransport = std::make_shared<P2PQuicPacketTransportIceAdapter>(P2PQuicPacketTransportIceAdapter(iceTransport->iceConnection()));
    ELOG_DEBUG("Create QuartcSessionConfig.");
    auto quartcSessionConfig = quic::QuartcSessionConfig();
    ELOG_DEBUG("P2PQuicTransport::create.");
    auto p2pQuicTransport = P2PQuicTransport::create(quartcSessionConfig, quic::Perspective::IS_SERVER, m_quicPacketTransport, quic::QuicChromiumClock::GetInstance(), m_alarmFactory, m_helper, serverCryptoConfig, m_compressedCertsCache.get());
    m_quicPacketTransport->SetDelegate(p2pQuicTransport.get());
    p2pQuicTransport->SetDelegate(this);
    return p2pQuicTransport;
}

void RTCQuicTransport::OnStream(P2PQuicStream* stream)
{
    assert(stream);
    ELOG_DEBUG("RTCQuicTransport::OnStream");
    {
        std::lock_guard<std::mutex> lock(m_streamQueueMutex);
        m_streamsToBeNotified.push(stream);
    }
    m_asyncOnStream.data = this;
    uv_async_send(&m_asyncOnStream);
}