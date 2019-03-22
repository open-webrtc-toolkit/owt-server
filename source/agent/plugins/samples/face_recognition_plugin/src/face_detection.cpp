// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <iterator>
#include <map>
#include <thread>

#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#include "face_detection.h"

using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;

BaseDetection::BaseDetection(std::string commandLineFlag, std::string topoName, int maxBatch)
      : commandLineFlag(commandLineFlag), topoName(topoName), maxBatch(maxBatch) {}

bool BaseDetection::enabled() const  {
      if (!enablingChecked) {
          _enabled = !commandLineFlag.empty();
          if (!_enabled) {
              cout << topoName << " DISABLED" << endl;
          }
          enablingChecked = true;
      }
      return _enabled;
}

void FaceDetectionClass::initialize(const std::string& model_xml_path, const std::string& device_name) {
    cout << "Initializing face_detection network..." << endl;
    std::map<std::string, InferenceEngine::InferencePlugin> pluginsForDevices;
    std::string device_for_faceDetection;
    std::string path_to_faceDetection_model;

    device_for_faceDetection = device_name;
    path_to_faceDetection_model = model_xml_path;

    cout << "FaceDetection plugin:" << device_for_faceDetection << endl;
    cout << "FaceDetection modle:" << path_to_faceDetection_model << endl;

    auto deviceName = device_for_faceDetection;
    auto networkName = path_to_faceDetection_model;

    // Need to provide the absolute path to the trained model*
    InferencePlugin infer_plugin = PluginDispatcher({""}).getPluginByDevice(deviceName);

    // Load extensions for the CPU plugin
    if (deviceName == "CPU") {
        infer_plugin.AddExtension(InferenceEngine::make_so_pointer<InferenceEngine::IExtension>(""));
    }

    pluginsForDevices[deviceName] = infer_plugin;

    this->net = infer_plugin.LoadNetwork(this->read(), {});
    this->plugin = &infer_plugin;
    cout << "Successfully loaded face_detection plugin." << endl;
}

void FaceDetectionClass::submitRequest(){
    if (!enquedFrames) return;
    enquedFrames = 0;
    resultsFetched = false;
    results.clear();
    BaseDetection::submitRequest();
}

void FaceDetectionClass::enqueue(const cv::Mat &frame) {
    if (!enabled()) return;

    if (!request) {
        request = net.CreateInferRequestPtr();
    }

    width = frame.cols;
    height = frame.rows;

    auto  inputBlob = request->GetBlob(input);

    matU8ToBlob<uint8_t >(frame, inputBlob);

    enquedFrames = 1;
}

void FaceDetectionClass::fetchResults() {
    if (!enabled()) return;
    results.clear();
    if (resultsFetched) return;
    resultsFetched = true;
    const float *detections = request->GetBlob(output)->buffer().as<float *>();

    for (int i = 0; i < maxProposalCount; i++) {
        float image_id = detections[i * objectSize + 0];
        Result r;
        r.label = static_cast<int>(detections[i * objectSize + 1]);
        r.confidence = detections[i * objectSize + 2];
        r.location.x = detections[i * objectSize + 3] * width;
        r.location.y = detections[i * objectSize + 4] * height;
        r.location.width = detections[i * objectSize + 5] * width - r.location.x;
        r.location.height = detections[i * objectSize + 6] * height - r.location.y;
        if (image_id < 0) {
            break;
        }
        results.push_back(r);
    }
}

InferenceEngine::CNNNetwork FaceDetectionClass::read() {
    cout << "Loading network files for Face Detection" << endl;
    InferenceEngine::CNNNetReader netReader;
    // Read network model
    netReader.ReadNetwork(commandLineFlag);
    // Set batch size to 1
    cout << "Batch size is set to  "<< maxBatch << endl;
    netReader.getNetwork().setBatchSize(maxBatch);
    // Extract model name and load it's weights
    std::string binFileName = fileNameNoExt(commandLineFlag) + ".bin";
    netReader.ReadWeights(binFileName);
    // Read labels (if any)
    std::string labelFileName = fileNameNoExt(commandLineFlag) + ".labels";

    std::ifstream inputFile(labelFileName);
    std::copy(std::istream_iterator<std::string>(inputFile),
              std::istream_iterator<std::string>(),
              std::back_inserter(labels));

    /** SSD-based network should have one input and one output **/
    // ---------------------------Check inputs ------------------------------------------------------
    cout << "Checking Face Detection inputs" << endl;
    InferenceEngine::InputsDataMap inputInfo(netReader.getNetwork().getInputsInfo());
    if (inputInfo.size() != 1) {
        throw std::logic_error("Face Detection network should have only one input");
    }
    auto& inputInfoFirst = inputInfo.begin()->second;
    inputInfoFirst->setPrecision(Precision::U8);
    inputInfoFirst->getInputData()->setLayout(Layout::NCHW);

    // ---------------------------Check outputs ------------------------------------------------------
    cout << "Checking Face Detection outputs" << endl;
    InferenceEngine::OutputsDataMap outputInfo(netReader.getNetwork().getOutputsInfo());
    if (outputInfo.size() != 1) {
        throw std::logic_error("Face Detection network should have only one output");
    }
    auto& _output = outputInfo.begin()->second;
    output = outputInfo.begin()->first;

    const auto outputLayer = netReader.getNetwork().getLayerByName(output.c_str());
    if (outputLayer->type != "DetectionOutput") {
        throw std::logic_error("Face Detection network output layer(" + outputLayer->name +
            ") should be DetectionOutput, but was " +  outputLayer->type);
    }

    if (outputLayer->params.find("num_classes") == outputLayer->params.end()) {
        throw std::logic_error("Face Detection network output layer (" +
            output + ") should have num_classes integer attribute");
    }

    const int num_classes = outputLayer->GetParamAsInt("num_classes");
    if (labels.size() != num_classes) {
        if (labels.size() == (num_classes - 1))  // if network assumes default "background" class, having no label
            labels.insert(labels.begin(), "fake");
        else
            labels.clear();
    }
    const InferenceEngine::SizeVector outputDims = _output->dims;
    maxProposalCount = outputDims[1];
    objectSize = outputDims[0];
    if (objectSize != 7) {
        throw std::logic_error("Face Detection network output layer should have 7 as a last dimension");
    }
    if (outputDims.size() != 4) {
        throw std::logic_error("Face Detection network output dimensions not compatible shoulld be 4, but was " +
                                       std::to_string(outputDims.size()));
    }
    _output->setPrecision(Precision::FP32);
    _output->setLayout(Layout::NCHW);
    input = inputInfo.begin()->first;
    cout <<"finished reading network" << endl;
    return netReader.getNetwork();
}


