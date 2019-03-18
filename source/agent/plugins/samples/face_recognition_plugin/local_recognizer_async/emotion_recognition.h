#include <string>
#include <chrono>
//#include <gflags/gflags.h>
#include <cmath>
#include <inference_engine.hpp>
#include <cpp/ie_plugin_cpp.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include <ext_list.hpp>

//#include <common_ie.hpp>
#include <iomanip>


class EmotionRecognitionClass
{
 	public:
	 	int initialize(std::string model_path);
	 	void load_frame(cv::Mat input_frame);
	 	std::vector<pair<string,float>> do_infer();
	 	~EmotionRecognitionClass();
	private:
 		//void prewhitten(); 
 		cv::Mat input_frame; 
 		float * input_buffer;
 		float * output_buffer;
 		cv::Mat* output_frames ;
 		cv::Mat frameInfer;
 		cv::Mat frameInfer_prewhitten;

 		InferenceEngine::CNNNetReader networkReader;
 		InferenceEngine::ExecutableNetwork executable_network;
 		InferenceEngine::InferencePlugin plugin;
 		InferenceEngine::InferRequest::Ptr   async_infer_request;
 		InferenceEngine::InputsDataMap input_info; 
 		InferenceEngine::OutputsDataMap output_info; 
}; 