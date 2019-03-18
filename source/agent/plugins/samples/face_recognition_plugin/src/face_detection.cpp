
#include "face_detection.h"
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <functional>
#include <fstream>
#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <map>
#include <inference_engine.hpp>

#include <common_ie.hpp>
//#include <slog.hpp>
#include "mkldnn/mkldnn_extension_ptr.hpp"
#include <ext_list.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <sys/stat.h>
#include <thread>

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
              std::cout << topoName << " DISABLED" << std::endl;
          }
          enablingChecked = true;
      }
      return _enabled;
}

void BaseDetection::printPerformanceCounts() {
      if (!enabled()) {
          return;
      }
      std::cout << "Performance counts for " << topoName << std::endl << std::endl;
      ::printPerformanceCounts(request->GetPerformanceCounts(), std::cout, false);
}

void FaceDetectionClass::initialize(){
    cout<<endl<<"==============Initialize face_detection network============"<<endl;
    std::map<std::string, InferenceEngine:: InferencePlugin> pluginsForDevices;
    std::string device_for_faceDetection;
    std::string path_to_faceDetection_model;
    std::vector<std::pair<std::string, std::string>> cmdOptions;

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
        device_for_faceDetection="HDDL";
        path_to_faceDetection_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP16/face-detection-retail-0004.xml";
    } else {
        device_for_faceDetection="GPU";
        path_to_faceDetection_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml";
    }

    printf("FaceDetection plugin: %s\n", device_for_faceDetection.c_str());
    printf("FaceDetection modle: %s\n", path_to_faceDetection_model.c_str());

    auto deviceName = device_for_faceDetection;
    auto networkName = path_to_faceDetection_model;

    //need to provide the absolute path to the trained model*
    InferencePlugin buffer_plugin = PluginDispatcher\
      ({""}).getPluginByDevice(deviceName);
    //InferencePlugin buffer_plugin = PluginDispatcher({"./lib/", ""}).getPluginByDevice(deviceName);

    //Print plugin version /
    printPluginVersion(buffer_plugin, std::cout);

    ///Load extensions for the CPU plugin
#if 0 
    //if ((deviceName.find(device_for_faceDetection) != std::string::npos)) {
        buffer_plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
    //}
#endif
    pluginsForDevices[deviceName] = buffer_plugin;

    //Load(FaceDetection).into(plugin);
    this->net= buffer_plugin.LoadNetwork(this->read(), {});
    this->plugin= &buffer_plugin;
    std::cout<<"==============successfully loaded face_detection plugin==========="<<std::endl<<endl;;

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
    std::cout<<"finished reading network"<<std::endl;
    return netReader.getNetwork();
}


