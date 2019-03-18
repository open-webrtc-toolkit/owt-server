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

#include "person_detection.h"
using namespace std;
using namespace cv;
using namespace InferenceEngine::details;
using namespace InferenceEngine;


PersonDetectionClass::~PersonDetectionClass(){
    delete [] output_frames;
}

int PersonDetectionClass::initialize(std::string modelfile){
    InferenceEngine::PluginDispatcher dispatcher({""});
    //InferencePlugin plugin(dispatcher.getPluginByDevice("CPU")); 
    plugin=dispatcher.getPluginByDevice("GPU"); 
 
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
    //plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());

    if (input_info.size() != 1) {
        cout << "This sample accepts networks having only one input." << endl;
        return 1;
    }

    for (auto &item : input_info) {
        auto input_data = item.second;
        input_data->setPrecision(Precision::FP32);
        input_data->setLayout(Layout::NCHW);
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
        output_data->setLayout(Layout::NCHW);
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

    Mat frame,frameInfer, frameInfer_prewhitten;

    // get the input blob buffer pointer location
   // free(input_buffer);
    input_buffer = NULL;
    for (auto &item : input_info) {
        auto input_name = item.first;
        auto input_data = item.second;
        auto input = async_infer_request->GetBlob(input_name);
        input_buffer = input->buffer().as<PrecisionTrait<Precision::FP32>::value_type *>();
    }

    // get the output blob pointer location

    output_buffer = NULL;
    for (auto &item : output_info) {
        auto output_name = item.first;
        auto output = async_infer_request->GetBlob(output_name);
        output_buffer = output->buffer().as<PrecisionTrait<Precision::FP32>::value_type *>();
    }

    output_frames = new Mat[1];
    return 0;
}

/*load a frame into the network, given that the network is already initialized*/
void PersonDetectionClass::load_frame(cv::Mat frame){
    const size_t input_width   = 544;
    const size_t input_height  = 320;
    const size_t output_width  = 1;
    const size_t output_height = 512;
    const size_t infer_width = 544;
    const size_t infer_height= 320;
    const size_t num_channels= 3;

    resize(frame, frameInfer, Size(infer_width, infer_height));

    //std::cout<<"output size"<< output_frames[0].rows<<std::endl;
    //std::cout<<"frameInfer input"<< frameInfer.rows<< "|"<<frameInfer.cols<<std::endl;

    auto input_channels = 3;  // channels for color format.  RGB=4   
    int input_size=3 * infer_width * infer_height; 
    size_t framesize = frameInfer.rows * frameInfer.step1();
    if (framesize != input_size) {
        cout << "input pixels mismatch, expecting " << input_size
                  << " bytes, got: " << framesize << endl;
    }

    int channel_size=infer_width * infer_height;
   // cout<<frameInfer_prewhitten<<endl<<"above frameInfer"<<endl;
    //cout<< "writing to blob" <<endl;
    for (size_t pixelnum = 0, imgIdx = 0; pixelnum < channel_size; ++pixelnum) {
        for (size_t ch = 0; ch < num_channels; ++ch) {
            //cout<<  (int)frameInfer.at<cv::Vec3b>(pixelnum)[ch] <<endl;
            input_buffer[(ch * channel_size) + pixelnum] = frameInfer.at<cv::Vec3b>(pixelnum)[ch];                    
        }
        //cout<<endl;
    }  

}

std::vector<float> PersonDetectionClass::do_infer(){
    int num_channels=3;
    int channel_size=25600;

    async_infer_request->StartAsync();
            

    async_infer_request->Wait(IInferRequest::WaitMode::RESULT_READY);
    //async_infer_request.Wait();
    std::vector<float> v1;
    
    int i=0;
    while (output_buffer[i]>-0.1){
        v1.push_back(output_buffer[i]);
        i++;
    } 


    
    return v1;
}


