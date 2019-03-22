// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef CNN_H
#define CNN_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <opencv2/opencv.hpp>
#include <inference_engine.hpp>

/**
* @brief Base class of config for network
*/
struct CnnConfig {
    explicit CnnConfig(const std::string& path_to_model,
                       const std::string& path_to_weights)
        : path_to_model(path_to_model), path_to_weights(path_to_weights) {}

    /** @brief Path to model description */
    std::string path_to_model;
    /** @brief Path to model weights */
    std::string path_to_weights;
    /** @brief Maximal size of batch */
    int max_batch_size{1};

    /** @brief Plugin to use for inference */
    InferenceEngine::InferencePlugin plugin;
};

/**
* @brief Base class of network
*/
class CnnDLSDKBase {
public:
    using Config = CnnConfig;

    /**
   * @brief Constructor
   */
    explicit CnnDLSDKBase(const Config& config);

    /**
   * @brief Descructor
   */
    ~CnnDLSDKBase() {}

    /**
   * @brief Loads network
   */
    void Load();

    /**
    * @brief Prints performance report
    */
    void PrintPerformanceCounts() const;

protected:
    /**
   * @brief Run network
   *
   * @param frame Input image
   * @param results_fetcher Callback to fetch inference results
   */
    void Infer(const cv::Mat& frame,
               std::function<void(const InferenceEngine::BlobMap&, size_t)> results_fetcher) const;

    /**
   * @brief Run network in batch mode
   *
   * @param frames Vector of input images
   * @param results_fetcher Callback to fetch inference results
   */
    void InferBatch(const std::vector<cv::Mat>& frames,
                    std::function<void(const InferenceEngine::BlobMap&, size_t)> results_fetcher) const;

    /** @brief Config */
    Config config_;
    /** @brief IE plugin */
    InferenceEngine::InferencePlugin net_plugin_;
    /** @brief Net inputs info */
    InferenceEngine::InputsDataMap inInfo_;
    /** @brief Net outputs info */
    InferenceEngine::OutputsDataMap outInfo_;
    /** @brief IE network */
    InferenceEngine::ExecutableNetwork executable_network_;
    /** @brief IE InferRequest */
    mutable InferenceEngine::InferRequest infer_request_;
    /** @brief Pointer to the pre-allocated input blob */
    mutable InferenceEngine::Blob::Ptr input_blob_;
    /** @brief Map of output blobs */
    InferenceEngine::BlobMap outputs_;
};

class VectorCNN : public CnnDLSDKBase {
public:
    explicit VectorCNN(const CnnConfig& config);

    void Compute(const cv::Mat& image,
                 cv::Mat* vector, cv::Size outp_shape = cv::Size()) const;
    void Compute(const std::vector<cv::Mat>& images,
                 std::vector<cv::Mat>* vectors, cv::Size outp_shape = cv::Size()) const;
};

class BaseCnnDetection {
protected:
    InferenceEngine::InferRequest::Ptr request;
    const bool isAsync;
    std::string topoName;

public:
    explicit BaseCnnDetection(bool isAsync = false) : isAsync(isAsync) {}

    virtual ~BaseCnnDetection() {}

    virtual void submitRequest() {
        if (!enabled() || request == nullptr) return;
        if (isAsync) {
            request->StartAsync();
        } else {
            request->Infer();
        }
    }

    virtual void wait() {
        if (!enabled()|| !request || !isAsync) return;
        request->Wait(InferenceEngine::IInferRequest::WaitMode::RESULT_READY);
    }

    bool enabled() const  {
        return true;
    }

    void PrintPerformanceCounts() {
        if (!enabled()) {
            return;
        }
    }
};

#endif
