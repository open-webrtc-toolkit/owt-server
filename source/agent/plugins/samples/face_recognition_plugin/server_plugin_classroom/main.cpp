// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <unistd.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <memory>
#include <vector>
#include <iterator>
#include <map>
#include <thread>
#include <mutex>

#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"

#include "action_detector.h"
#include "myplugin.h"

using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;

// Globals
std::mutex mtx;
ActionDetectionClass ActionDetection;
cv::Mat mGlob(1280, 720, CV_8UC3);
vector<action_result> global_action_results;
volatile bool need_process = false;

// This gets the label name map.
std::string GetActionTextLabel(const unsigned label) {
    static std::vector<std::string> actions_map = {"sitting", "standing",
                                                   "raising_hand"};
    if (label < actions_map.size()) {
        return actions_map[label];
    }
    return "__undefined__";
}

threading_class::threading_class(){}

void threading_class::make_thread(){
    std::thread T = std::thread(&threading_class::threading_func, this);
    T.detach();
}

void get_final_actions(){
    Mat frame;
    mtx.lock();
    frame = mGlob.clone();
    mtx.unlock();

    DetectedActions actions;
    ActionDetection.enqueue(frame);
    ActionDetection.submitRequest();
    ActionDetection.wait();
    ActionDetection.fetchResults();

    global_action_results.clear();
    for (auto p : ActionDetection.results) {
      action_result result = {p.rect, p.detection_conf, GetActionTextLabel(p.label)};
      global_action_results.push_back(result);
    }
}

// Async inference function
void threading_class::threading_func(){
    ActionDetectorConfig action_config;
    action_config.is_async = false;
    action_config.detection_confidence_threshold = 0.35;
    action_config.path_to_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-action-recognition-0004/FP32/person-detection-action-recognition-0004.xml";
    action_config.path_to_weights = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-action-recognition-0004/FP32/person-detection-action-recognition-0004.bin";
    ActionDetection.initialize(action_config);

    while (true){
        if (need_process){
            mtx.lock();
            Mat frame = mGlob.clone();
            mtx.unlock();

            get_final_actions();
        } else {
          usleep(5*1000);
        }
    }
}

MyPlugin::MyPlugin(){}

rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
    threading_class t_c;
    t_c.make_thread();
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::PluginClose() {
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {
    // Fetch data from the frame and do face detection using the inference engine plugin
    // after update, send it back to analytics server.
    if (!buffer->buffer) {
        return RVA_ERR_OK;
    }

    if (buffer->width >= 320 && buffer->height >= 240) {
        clock_t begin = clock();
        cv::Mat mYUV(buffer->height + buffer->height / 2, buffer->width, CV_8UC1, buffer->buffer );
        cv::Mat mBGR(buffer->height, buffer->width, CV_8UC3);
        cv::cvtColor(mYUV, mBGR, cv::COLOR_YUV2BGR_I420);
        // Update the mat for inference, and fetch the latest result
        mtx.lock();
        if (mBGR.cols > 0 && mBGR.rows > 0){
            mGlob = mBGR.clone();
            need_process = true;
        } else {
            cout << "One mBGR is not qualified for inference" << endl;
        }
        mtx.unlock();

        // Draw the recongnition results
        mtx.lock();
        for(auto result : global_action_results){
            rectangle(mBGR,result.rectangle,Scalar(18, 255, 255), 2);
            putText(mBGR,
                        "Action : " + result.label ,
                        cv::Point2f(result.rectangle.x,result.rectangle.y - 20),
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(167, 255, 73), 1, CV_AA);
            putText(mBGR,
                        "Precision : " + to_string(result.value) ,
                        cv::Point2f(result.rectangle.x, result.rectangle.y - 3),
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(167, 255, 73), 1, CV_AA);
        }

        mtx.unlock();
        cv::cvtColor(mBGR, mYUV, cv::COLOR_BGR2YUV_I420);

        buffer->buffer = mYUV.data;
        clock_t end = clock();
    }
    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }
    return RVA_ERR_OK;
}

// Declare the plugin
DECLARE_PLUGIN(MyPlugin)

