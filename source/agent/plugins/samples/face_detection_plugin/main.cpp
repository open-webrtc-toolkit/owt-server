// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <iostream>
#include <iterator>
#include <thread>
#include "opencv2/imgproc/imgproc.hpp"

#include "detectionconfig.h"
#include "myplugin.h"

using namespace InferenceEngine;

// Globals
std::mutex mtx;
FaceDetectionClass FaceDetection;
std::map<std::string, InferenceEngine:: InferencePlugin> pluginsForDevices;
std::string device_for_faceDetection;
std::string path_to_faceDetection_model;
std::vector<std::pair<std::string, std::string>> cmdOptions;
cv::Mat mGlob(1280,720,CV_8UC3);
Result Glob_result;
volatile bool need_process =false;

BaseDetection::BaseDetection(std::string commandLineFlag, std::string topoName, int maxBatch)
      : commandLineFlag(commandLineFlag), topoName(topoName), maxBatch(maxBatch) {}

bool BaseDetection::enabled() const  {
      if (!enablingChecked) {
          _enabled = !commandLineFlag.empty();
          if (!_enabled) {
              std::cout << topoName << " DISABLED" << std::endl;
          }
          enablingChecked = true;
      }
      return _enabled;
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
    std::cout << "Loading network files for Face Detection" << std::endl;
    InferenceEngine::CNNNetReader netReader;
    /** Read network model **/
    netReader.ReadNetwork(commandLineFlag);
    /** Set batch size to 1 **/
    std::cout << "Batch size is set to  "<< maxBatch << std::endl;
    netReader.getNetwork().setBatchSize(maxBatch);
    /** Extract model name and load it's weights **/
    std::string binFileName = fileNameNoExt(commandLineFlag) + ".bin";
    netReader.ReadWeights(binFileName);
    /** Read labels (if any)**/
    std::string labelFileName = fileNameNoExt(commandLineFlag) + ".labels";

    std::ifstream inputFile(labelFileName);
    std::copy(std::istream_iterator<std::string>(inputFile),
              std::istream_iterator<std::string>(),
              std::back_inserter(labels));
    // -----------------------------------------------------------------------------------------------------

    /** SSD-based network should have one input and one output **/
    // ---------------------------Check inputs ------------------------------------------------------
    std::cout << "Checking Face Detection inputs" << std::endl;
    InferenceEngine::InputsDataMap inputInfo(netReader.getNetwork().getInputsInfo());
    if (inputInfo.size() != 1) {
        throw std::logic_error("Face Detection network should have only one input");
    }
    auto& inputInfoFirst = inputInfo.begin()->second;
    inputInfoFirst->setPrecision(Precision::U8);
    inputInfoFirst->getInputData()->setLayout(Layout::NCHW);
    // -----------------------------------------------------------------------------------------------------

    // ---------------------------Check outputs ------------------------------------------------------
    std::cout << "Checking Face Detection outputs" << std::endl;
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
    std::cout << "finished reading network" << std::endl;
    return netReader.getNetwork();
}


void Load::into(InferenceEngine::InferencePlugin & plg) const {
    if (detector.enabled()) {
        detector.net = plg.LoadNetwork(detector.read(), {});
        detector.plugin = &plg;
        std::cout << "Successfully loaded inference plugin" << std::endl;
    }
}


threading_class::threading_class(){}

void threading_class::make_thread(){
  std::thread hi=std::thread(&threading_class::threading_func, this);
  hi.detach();
}

//------------calling function for the new thread-------------------
void threading_class::threading_func(){
    for (auto && option : cmdOptions) {
      auto deviceName = option.first;
      auto networkName = option.second;

      if (pluginsForDevices.find(deviceName) != pluginsForDevices.end()) {
          continue;
      }

      std::cout << "Loading inference plugin " << deviceName << std::endl;
      InferencePlugin plugin = PluginDispatcher({""}).getPluginByDevice(deviceName);

      /** Load extensions for the CPU plugin **/
      if ((deviceName.find(device_for_faceDetection) != std::string::npos) && deviceName == "CPU") {
          plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
      } 
      pluginsForDevices[deviceName] = plugin;
    }
    Load(FaceDetection).into(pluginsForDevices[device_for_faceDetection]);

    std::cout<<"creating new thread for inferecne async"<<std::endl;
    while (true){
        if (need_process){
          mtx.lock();
          cv::Mat mInput = mGlob.clone();
          mtx.unlock();
          FaceDetection.enqueue(mInput);
          FaceDetection.submitRequest();
          FaceDetection.wait();
          FaceDetection.fetchResults(); 
          mtx.lock();
          if (FaceDetection.results.size() > 0){
              Glob_result = FaceDetection.results.front();
              need_process = false;
          }   
          mtx.unlock();
        }
    }
}

MyPlugin::MyPlugin()
  : frame_callback(nullptr)
  , event_callback(nullptr) {
}



void MyPlugin::load_init()
{
}


rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
      std::cout << "InferenceEngine version: " << InferenceEngine::GetInferenceEngineVersion() << std::endl;
      // Initialize the plugin
      device_for_faceDetection = "GPU";
      path_to_faceDetection_model = FaceDetection.commandLineFlag;
      cmdOptions = { {device_for_faceDetection, path_to_faceDetection_model} }; 
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
    if (buffer->width >=320 && buffer->height >=240) {   
        clock_t begin = clock();
        
        cv::Mat mYUV(buffer->height + buffer->height/2, buffer->width, CV_8UC1, buffer->buffer );
        cv::Mat mBGR(buffer->height, buffer->width, CV_8UC3);
        cv::cvtColor(mYUV,mBGR,cv::COLOR_YUV2BGR_I420);
        // Update the mat for inference, and fetch the latest result
        mtx.lock(); 
          if (mBGR.cols > 0 && mBGR.rows > 0){
              mGlob = mBGR.clone();
              need_process = true;
          } else {
              std::cout << "one mBGR is not qualified for inference" << std::endl;
          }
        mtx.unlock(); 

        std::ostringstream out;
        out.str("Face Detection Results");
        // Draw the detection results
        mtx.lock();
        if (Glob_result.confidence>0.65){
            cv::putText(mBGR,
                out.str(),
                cv::Point2f(Glob_result.location.x, Glob_result.location.y - 15),
                cv::FONT_HERSHEY_COMPLEX_SMALL,
                1.3,
                cv::Scalar(0, 0, 255));
            cv::rectangle(mBGR, Glob_result.location, cv::Scalar(255, 0, 0), 1);
        }  
        mtx.unlock();
        cv::cvtColor(mBGR,mYUV,cv::COLOR_BGR2YUV_I420);     
        //------------------------return the frame with 
        buffer->buffer=mYUV.data;
        clock_t end = clock();      
    }

    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }

    if (event_callback) {
        std::stringstream ss;
        ss << "x:" << Glob_result.location.x << "y:" << Glob_result.location.y << "w:" << Glob_result.location.width
           << "h:" << Glob_result.location.height;
        std::string msg = ss.str();
        event_callback->OnPluginEvent(0, msg);
    }
    return RVA_ERR_OK;
} 

// Declare the plugin 
DECLARE_PLUGIN(MyPlugin)

