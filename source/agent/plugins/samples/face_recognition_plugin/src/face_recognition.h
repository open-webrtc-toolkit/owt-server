#include <string>
#include <chrono>
#include <cmath>
#include <inference_engine.hpp>
#include <cpp/ie_plugin_cpp.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <iomanip>

class FaceRecognitionClass
{
    public:
        int initialize(std::string model_path);
        void load_frame(cv::Mat input_frame);
        std::vector<float> do_infer();
        ~FaceRecognitionClass();
    private:
        unsigned char *input_buffer;
        float * output_buffer;

        InferenceEngine::CNNNetReader networkReader;
        InferenceEngine::ExecutableNetwork executable_network;
        InferenceEngine::InferencePlugin plugin;
        InferenceEngine::InferRequest::Ptr   async_infer_request;
        InferenceEngine::InputsDataMap input_info;
        InferenceEngine::OutputsDataMap output_info;
};
