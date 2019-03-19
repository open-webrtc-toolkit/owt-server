/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

/*
// brief This tutorial replaces OpenCV DNN with IE
*/

#include <string>
#include <chrono>
#include <cmath>
#include <inference_engine.hpp>
#include <cpp/ie_plugin_cpp.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include <ext_list.hpp>

#include <iomanip>

#include "face_recognition.h"
using namespace std;
using namespace cv;
using namespace InferenceEngine::details;
using namespace InferenceEngine;

FaceRecognitionClass::~FaceRecognitionClass(){
}

int FaceRecognitionClass::initialize(std::string modelfile){
    InferenceEngine::PluginDispatcher dispatcher({""});

    cout<< "============Initialize FaceRecognition ================="<<endl;

    std::string device;
    std::string path_to_faceDetection_model;

    bool using_hddl = false;

#if 0
    {
        const char *env_hddl = "ANALYTICS_HDDL";
        char *p = NULL;

        p = getenv(env_hddl);
        if (p) {
            printf("%s: %s\n", env_hddl, p);

            if (atoi(p) > 0)
                using_hddl = true;
        } else {
            printf("%s: %s\n", env_hddl, "NOT_SET");
        }
    }
#endif

    if (using_hddl) {
        device="HDDL";
        path_to_faceDetection_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-reidentification-retail-0001/FP16/face-reidentification-retail-0001.xml";
    } else {
        device="GPU";
        path_to_faceDetection_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-reidentification-retail-0001/FP32/face-reidentification-retail-0001.xml";
    }

    printf("FaceRecognition plugin: %s\n", device.c_str());
    printf("FaceRecognition modle: %s\n", path_to_faceDetection_model.c_str());

    plugin=dispatcher.getPluginByDevice(device);
    modelfile = path_to_faceDetection_model;

    try {
        networkReader.ReadNetwork(modelfile);
    }
    catch (InferenceEngineException ex) {
        cerr << "Failed to load network: "  << endl;
        return 1;
    }

    cout << "Network loaded." << endl;
    auto pos=modelfile.rfind('.');
    if (pos !=string::npos) {
        string binFileName=modelfile.substr(0,pos)+".bin";
        std::cout<<"binFileName="<<binFileName<<std::endl;
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

    cout << "Setting-up input, output blobs..." << endl;

    // ---------------
    // set input configuration
    // ---------------
    //InferenceEngine::InputsDataMap input_info(network.getInputsInfo());
    input_info=network.getInputsInfo();
    InferenceEngine::SizeVector inputDims;
#if 0
    plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
#endif

    if (input_info.size() != 1) {
        cout << "This sample accepts networks having only one input." << endl;
        return 1;
    }

    for (auto &item : input_info) {
        auto input_data = item.second;
        input_data->setPrecision(Precision::U8);
        //input_data->setLayout(Layout::NCHW);
        inputDims=input_data->getDims();
    }
    cout << "inputDims=";
    for (int i=0; i<inputDims.size(); i++) {
        cout << (int)inputDims[i] << " ";
    }
    cout << endl;

    const int infer_width=inputDims[0];
    const int infer_height=inputDims[1];
    const int num_channels=inputDims[2];
    const int channel_size=infer_width*infer_height;
    const int full_image_size=channel_size*num_channels;

    /** Get information about topology outputs **/
    output_info=network.getOutputsInfo();
    InferenceEngine::SizeVector outputDims;
    for (auto &item : output_info) {
        auto output_data = item.second;
        output_data->setPrecision(Precision::FP32);
        //output_data->setLayout(Layout::NCHW);
        outputDims=output_data->getDims();
    }
    cout << "outputDims=";
    for (int i=0; i<outputDims.size(); i++) {
        cout << (int)outputDims[i] << " ";
    }
    cout << endl;

    const int output_data_size=outputDims[1]*outputDims[2]*outputDims[3];

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
       cout << "____________________________Failed to create async infer req." << std::endl;
    }

    //----------------------------------------------------------------------------
    //  Inference engine output setup
    //----------------------------------------------------------------------------

    // get the input blob buffer pointer location
   // free(input_buffer);
    input_buffer = NULL;
    for (auto &item : input_info) {
        auto input_name = item.first;
        auto input_data = item.second;
        auto input = async_infer_request->GetBlob(input_name);
        input_buffer = input->buffer().as<PrecisionTrait<Precision::U8>::value_type *>();
    }

    // get the output blob pointer location

    output_buffer = NULL;
    for (auto &item : output_info) {
        auto output_name = item.first;
        auto output = async_infer_request->GetBlob(output_name);
        output_buffer = output->buffer().as<PrecisionTrait<Precision::FP32>::value_type *>();
    }

    cout<< "==========Sucessfully initialized face_recognition plugin==============="<<endl;
    return 0;
}

/*load a frame into the network, given that the network is already initialized*/
void FaceRecognitionClass::load_frame(cv::Mat frame){
    if (frame.size().width != 128 || frame.size().height != 128) {
        cout << "Error, invalid frame size, " << frame.size().width << " x " << frame.size().height << endl;
        return;
    }

    size_t num_channels = 3;
    size_t image_size = 128 * 128;

    for (size_t ch = 0; ch < num_channels; ch++) {
        /** Iterate over all channels **/
        for (size_t pid = 0; pid < image_size; pid++) {
            /** [images stride + channels stride + pixel id ] all in bytes **/
            input_buffer[ch * image_size + pid] = frame.at<unsigned char>(pid*num_channels + ch);
        }
    }
}

std::vector<float> FaceRecognitionClass::do_infer(){
    async_infer_request->StartAsync();
    async_infer_request->Wait(IInferRequest::WaitMode::RESULT_READY);
    //async_infer_request.Wait();

    std::vector<float> v1;
    for (int i = 0; i < 128; ++i)
    {
        float *localbox=&output_buffer[i];
        //std::cout<<(float)localbox[0]<<" ";
        /*!!!!!!!!! chage back to localbox0*/
        v1.push_back(localbox[0]);
    }
    return v1;
}


