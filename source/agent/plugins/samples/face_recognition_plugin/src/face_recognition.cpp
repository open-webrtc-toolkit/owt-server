// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "face_recognition.h"

using namespace std;
using namespace cv;
using namespace InferenceEngine::details;
using namespace InferenceEngine;

FaceRecognitionClass::~FaceRecognitionClass() {
}

int FaceRecognitionClass::initialize(const std::string& model_xml_path, const std::string& device_name) {
    cout << "Initializing FaceRecognition..." << endl;
    InferenceEngine::PluginDispatcher dispatcher({""});
    std::string device = device_name;
    std::string modelfile = model_xml_path;

    cout << "FaceRecognition plugin:" << device << endl;;
    cout << "FaceRecognition modle:" << model_xml_path << endl;

    plugin=dispatcher.getPluginByDevice(device);

    try {
        networkReader.ReadNetwork(modelfile);
    } catch (InferenceEngineException ex) {
        cerr << "Failed to load network: "  << endl;
        return 1;
    }

    cout << "Network loaded." << endl;
    auto pos = modelfile.rfind('.');
    if (pos != string::npos) {
        string binFileName = modelfile.substr(0, pos) + ".bin";
        cout << "binFileName=" << binFileName << endl;
        networkReader.ReadWeights(binFileName.c_str());
    }
    else {
        cerr << "Failed to load weights: " << endl;
        return 1;
    }

    auto network = networkReader.getNetwork();

    // --------------------
    // Set batch size
    // --------------------
    networkReader.getNetwork().setBatchSize(1);
    size_t batchSize = network.getBatchSize();
    cout << "Batch size = " << batchSize << endl;

    //----------------------------------------------------------------------------
    //  Inference engine input setup
    //----------------------------------------------------------------------------
    // ---------------
    // set input configuration
    // ---------------
    input_info = network.getInputsInfo();
    InferenceEngine::SizeVector inputDims;

    if (device == "CPU") {
        plugin.AddExtension(InferenceEngine::make_so_pointer<InferenceEngine::IExtension>(""));
    }

    if (input_info.size() != 1) {
        cout << "This sample accepts networks having only one input." << endl;
        return 1;
    }

    for (auto &item : input_info) {
        auto input_data = item.second;
        input_data->setPrecision(Precision::U8);
        inputDims = input_data->getDims();
    }

    infer_width = inputDims[2];
    infer_height = inputDims[3];
    num_channels = inputDims[1];

    channel_size = infer_width * infer_height;

    // Get information about topology outputs
    output_info = network.getOutputsInfo();
    InferenceEngine::SizeVector outputDims;
    for (auto &item : output_info) {
        auto output_data = item.second;
        output_data->setPrecision(Precision::FP32);
        outputDims = output_data->getDims();
    }

    const int output_size = outputDims[1];

    // --------------------------------------------------------------------------
    // Load model into plugin
    // --------------------------------------------------------------------------
    cout << "Loading model to plugin..." << endl;

    executable_network = plugin.LoadNetwork(network, {});

    // --------------------------------------------------------------------------
    // Create infer request
    // --------------------------------------------------------------------------
    cout << "Create infer request..." << endl;

    async_infer_request = executable_network.CreateInferRequestPtr();

    if (async_infer_request == nullptr) {
       cout << "Failed to create async infer req." << endl;
    }

    //----------------------------------------------------------------------------
    //  Inference engine output setup
    //----------------------------------------------------------------------------

    // Get the input blob buffer pointer location
    input_buffer = nullptr;
    for (auto &item : input_info) {
        auto input_name = item.first;
        auto input_data = item.second;
        auto input = async_infer_request->GetBlob(input_name);
        input_buffer = input->buffer().as<PrecisionTrait<Precision::U8>::value_type *>();
    }

    // Get the output blob pointer location
    output_buffer = nullptr;
    for (auto &item : output_info) {
        auto output_name = item.first;
        auto output = async_infer_request->GetBlob(output_name);
        output_buffer = output->buffer().as<PrecisionTrait<Precision::FP32>::value_type *>();
    }

    cout << "Sucessfully initialized face_recognition plugin."<<endl;
    return 0;
}

// Load a frame into the network, given that the network is already initialized
void FaceRecognitionClass::load_frame(cv::Mat frame){
    if (infer_width == 0 || infer_height == 0)
        return;

    if (frame.size().width != infer_width || frame.size().height != infer_height) {
        cout << "Error, invalid frame size, "
             << frame.size().width << " x "
             << frame.size().height
             << endl;
        return;
    }

    size_t image_size = infer_width * infer_height;

    for (size_t ch = 0; ch < num_channels; ch++) {
        // Iterate over all channels
        for (size_t pid = 0; pid < image_size; pid++) {
            input_buffer[ch * image_size + pid] = frame.at<unsigned char>(pid * num_channels + ch);
        }
    }
}

std::vector<float> FaceRecognitionClass::do_infer(){
    async_infer_request->StartAsync();
    async_infer_request->Wait(IInferRequest::WaitMode::RESULT_READY);

    std::vector<float> v1;
    for (int i = 0; i < output_size; ++i)
    {
        float *localbox = &output_buffer[i];
        v1.push_back(localbox[0]);
    }
    return v1;
}


