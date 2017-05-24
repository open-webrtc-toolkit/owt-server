/*
 * SDPProcessor.h
 */

#ifndef SDPINFO_H_
#define SDPINFO_H_

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

#include <logger.h>

namespace erizo {
/**
 * ICE candidate types
 */
enum HostType {
    HOST, SRFLX, PRFLX, RELAY
};
/**
 * Channel types
 */
enum MediaType {
    VIDEO_TYPE, AUDIO_TYPE, OTHER
};
/**
 * RTP Profile
 */
enum Profile {
  AVPF, SAVPF
};
/**
 * SRTP info.
 */
class CryptoInfo {
public:
    CryptoInfo() :
            tag(0) {
    }
    /**
     * tag number
     */
    int tag;
    /**
     * The cipher suite. Only AES_CM_128_HMAC_SHA1_80 is supported as of now.
     */
    std::string cipherSuite;
    /**
     * The key
     */
    std::string keyParams;
    /**
     * The MediaType
     */
    MediaType mediaType;

};
/**
 * Contains the information of an ICE Candidate
 */
class CandidateInfo {
public:
    CandidateInfo() : tag(0), generation(-1) {}
    bool isBundle;
    int tag;
    unsigned int priority;
    unsigned int componentId;
    std::string foundation;
    std::string hostAddress;
    std::string rAddress;
    int hostPort;
    int rPort;
    std::string netProtocol;
    HostType hostType;
    std::string transProtocol;
    std::string username;
    std::string password;
    MediaType mediaType;
    int generation;  // -1 if this info does not exist.
};

/**
 * A bundle tag
 */
class BundleTag {
  public:
    BundleTag(std::string theId, MediaType theType):id(theId),mediaType(theType){
    };
    std::string id;
    MediaType mediaType;
};

/**
 * A PT to Codec map
 */
struct RtpMap {
  unsigned int payloadType;
  std::string encodingName;
  unsigned int clockRate;
  MediaType mediaType;
  unsigned int channels;
  bool enable;
  std::map<std::string, std::string> formatParameters;
};

enum StreamDirection {
  UNSPECIFIED,
  RECVONLY,
  SENDONLY,
  SENDRECV
};

/**
 * Contains the information of a single SDP.
 * Used to parse and generate SDPs
 * SdpInfo is thread safe.
 */
class SdpInfo {
    DECLARE_LOGGER();
public:
    /**
     * Constructor
     */
    SdpInfo();
    virtual ~SdpInfo();
    /**
     * Inits the object with a given SDP.
     * @param sdp An string with the SDP.
     * @return true if success
     */
    bool initWithSdp(const std::string& sdp, const std::string& media);
    /**
     * Adds a new candidate.
     * @param info The CandidateInfo containing the new candidate
     */
    std::string addCandidate(const CandidateInfo& info);
    /**
     * Adds SRTP info.
     * @param info The CryptoInfo containing the information.
     */
    void addCrypto(const CryptoInfo& info);
    /**
     * Gets the candidates.
     * @return A vector containing the current candidates.
     */
    std::vector<CandidateInfo> getCandidateInfos() const;
    /**
     * Gets the SRTP information.
     * @return A vector containing the CryptoInfo objects with the SRTP information.
     */
    std::vector<CryptoInfo> getCryptoInfos() const;
    /**
    * Gets the payloadType information
    * @return A vector containing the PT-codec information
    */
    std::list<RtpMap> getPayloadInfos() const;
    /**
     * Sets the payloadType information.
     * @param info A vector containing the PT-codec information
     */
    void setPayloadInfos(const std::list<RtpMap>& info);
    /**
     * Gets the actual SDP.
     * @return The SDP in string format.
     */
    std::string getSdp();
    /**
     * @brief map external payload type to an internal id
     * @param externalPT The audio payload type as coming from this source
     * @return The internal payload id
     */
    int getAudioInternalPT(int externalPT);
    /**
     * @brief map external payload type to an internal id
     * @param externalPT The video payload type as coming from this source
     * @return The internal payload id
     */
    int getVideoInternalPT(int externalPT);
    /**
     * @brief map internal payload id to an external payload type
     * @param internalPT The payload type id used internally
     * @return The external payload type as provided to this source
     */
    int getAudioExternalPT(int internalPT);
    /**
     * @brief map internal payload it to an external payload type
     * @param internalPT The payload id as used internally
     * @return The external video payload type
     */
    int getVideoExternalPT(int internalPT);

    void setCredentials(const std::string& username, const std::string& password, MediaType media);

    void getCredentials(std::string& username, std::string& password, MediaType media);

    std::unique_ptr<RtpMap> getCodecByName(const std::string codecName, const unsigned int clockRate);

    bool supportCodecByName(const std::string codecName, const unsigned int clockRate);

    int forceCodecSupportByName(const std::string codecName, const unsigned int clockRate, MediaType mediaType);

    bool supportPayloadType(const int payloadType);

    bool supportNACK() { return nackEnabled; }
    void setNACKSupport(bool enable) { nackEnabled = enable; }
    bool setREDSupport(bool enable);
    bool setFECSupport(bool enable);
    bool removePayloadSupport(unsigned int payloadType);
    int preferredPayloadType(MediaType);

    /**
     * @brief copies relevant information from the offer sdp for which this will be an answer sdp
     * @param offerSdp The offer SDP as received via signaling and parsed
     */
    void setOfferSdp(const SdpInfo& offerSdp);

    /**
     * The audio and video SSRCs for this particular SDP.
     */
    unsigned int audioSsrc, videoSsrc, videoRtxSsrc;
    /**
    * Is it Bundle
    */
    bool isBundle;
    /**
    * Has audio
    */
    bool hasAudio;
    /**
    * Has video
    */
    bool hasVideo;
    /**
    * Has NACK support
    */
    bool nackEnabled;
    /**
    * Video direction
    */
    StreamDirection videoDirection;
    /**
    * Audio direction
    */
    StreamDirection audioDirection;
    /**
    * Is there rtcp muxing
    */
    bool isRtcpMux;

    /**
    * RTP Profile type
    */
    Profile profile;
    /**
    * Is there DTLS fingerprint
    */
    bool isFingerprint;
    /**
    * DTLS Fingerprint
    */
    std::string fingerprint;
    /**
    * Mapping from internal PT (key) to external PT (value)
    */
    std::map<const int, int> inOutPTMap;
    /**
    * Mapping from external PT (key) to intermal PT (value)
    */
    std::map<const int, int> outInPTMap;
    std::vector<BundleTag> bundleTags;
    /*
     * MLines for video and audio
     */
    int videoSdpMLine;
    int audioSdpMLine;
    int videoCodecs, audioCodecs;

private:
    bool processSdp(const std::string& sdp, const std::string& media);
    bool processCandidate(std::vector<std::string>& pieces, MediaType mediaType);
    std::string stringifyCandidate(const CandidateInfo & candidate);
    void gen_random(char* s, int len);
    std::vector<CandidateInfo> candidateVector_;
    mutable std::mutex candidateVectorMutex_;
    std::vector<CryptoInfo> cryptoVector_;
    mutable std::mutex cryptoVectorMutex_;
    std::list<RtpMap> payloadVector_;
    mutable std::mutex payloadVectorMutex_;
    std::vector<RtpMap> internalPayloadVector_;
    mutable std::mutex internalPayloadVectorMutex_;
    std::string iceVideoUsername_, iceAudioUsername_;
    std::string iceVideoPassword_, iceAudioPassword_;
    char msid[11];
};
}/* namespace erizo */
#endif /* SDPPROCESSOR_H_ */
