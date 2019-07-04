// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _COMMON_H
#define _COMMON_H

#include <sys/time.h>
#include <iostream>

#include <opencv2/opencv.hpp>

#include <inference_engine.hpp>

#define dout __now();std::cout << "  - " << "Plugin" << " - " << __FUNCTION__ << ":" << __LINE__ << " - "

void __now();
bool loadModel(std::string m_device, std::string m_model, InferenceEngine::InferencePlugin *m_plugin, InferenceEngine::InferRequest *m_infer_request, InferenceEngine::Blob::Ptr *m_input_blob, InferenceEngine::Blob::Ptr *m_output_blob);

template <typename T>
void matU8ToBlob(const cv::Mat& orig_image, InferenceEngine::Blob::Ptr& blob, float scaleFactor = 1.0, int batchIndex = 0) {
    InferenceEngine::SizeVector blobSize = blob.get()->dims();
    const size_t width = blobSize[0];
    const size_t height = blobSize[1];
    const size_t channels = blobSize[2];
    T* blob_data = blob->buffer().as<T*>();

    cv::Mat resized_image(orig_image);
    if (width != orig_image.size().width || height!= orig_image.size().height) {
        cv::resize(orig_image, resized_image, cv::Size(width, height));
    }

    int batchOffset = batchIndex * width * height * channels;

    for (size_t c = 0; c < channels; c++) {
        for (size_t  h = 0; h < height; h++) {
            for (size_t w = 0; w < width; w++) {
                blob_data[batchOffset + c * width * height + h * width + w] =
                    resized_image.at<cv::Vec3b>(h, w)[c] * scaleFactor;
            }
        }
    }
}

#endif //_COMMON_H
