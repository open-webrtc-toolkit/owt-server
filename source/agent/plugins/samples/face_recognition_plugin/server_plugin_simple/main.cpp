// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <iterator>
#include <map>
#include <thread>
#include <mutex>
#include <sys/time.h>

#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"

#include "face_detection.h"
#include "face_recognition.h"

#include "myplugin.h"
#include "pluginconfig.h"

using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;

// Globals
std::vector<std::pair<std::string, std::vector<float> > > data;
std::mutex mtx;
FaceDetectionClass FaceDetection;
FaceRecognitionClass FaceRecognition;
cv::Mat mGlob(1280,720,CV_8UC3);
vector<recog_result> global_recog_results;
volatile bool need_process = false;
float g_threshold = 0.45;
int infer_width = 0;
int output_size = 0;

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

float ComputeReidDistance(const cv::Mat& descr1, const cv::Mat& descr2) {
    float xy = descr1.dot(descr2);
    float xx = descr1.dot(descr1);
    float yy = descr2.dot(descr2);
    float norm = sqrt(xx * yy) + 1e-6;
    return 1.0f - xy / norm;
}

Mat get_cropped(Mat input, Rect rectangle , int crop_size , int style){
    int center_x = rectangle.x + rectangle.width / 2;
    int center_y = rectangle.y + rectangle.height / 2;

    int max_crop_size = min(rectangle.width , rectangle.height);

    int adjusted = max_crop_size * 3 / 2;

    std::vector<int> good_to_crop;
    good_to_crop.push_back(adjusted / 2);
    good_to_crop.push_back(input.size().height - center_y);
    good_to_crop.push_back(input.size().width - center_x);
    good_to_crop.push_back(center_x);
    good_to_crop.push_back(center_y);

    int final_crop = *(min_element(good_to_crop.begin(), good_to_crop.end()));

    Rect pre (center_x - max_crop_size / 2, center_y - max_crop_size / 2 , max_crop_size , max_crop_size  );
    Rect pre2 (center_x - final_crop, center_y - final_crop , final_crop * 2  , final_crop * 2  );

    Mat r;
    if (style == 0) {
        r = input (pre);
    } else {
        r = input (pre2);
    }

    resize(r,r,Size(crop_size,crop_size));
    return r;
}

// Read the vector saved by pre-process tool
vector<pair<string, vector<float> > > read_text(){
    cout << "Reading vectors of saved person" << endl;
    vector<pair<string, vector<float> > > data;
    string file = "./vectors.txt";
    string line;
    std::ifstream infile(file);
    int count;
    float value ;
    vector <float> vec;
    string name;
    while (std::getline(infile, line))
    {
        name =string(line);
        cout << name ;
        getline(infile , line);
        count= atoi(line.c_str());
        cout <<" : " << count <<endl;
        for (int i = 0; i < count; ++i)
        {
            vec.clear();
            for (int j = 0; j < output_size; ++j)
            {
                getline(infile , line);
                value = strtof(line.c_str(),0);
                vec.push_back(value);
            }
            data.push_back(pair<string,vector<float> > (name, vec));
        }

        for (size_t i = 0; i < 16; i++) {
            for (size_t j = 0; j < 16; j++) {
                cout << "   " << fixed << setprecision(2) << vec[i * 16 + j];
            }
            cout << endl;
        }
    }
    return data;
}

// Requires: data and a person(target) to be recognized
// Returns: distance and the name of the best-match person
//       If distance is higher than the threshold, the name is "Unknown"
pair<float , string> recognize(vector<pair<string, vector<float> > > *data , vector <float> target){
    float dist = 0;
    float min_value = 100;
    string best_match = "Unknown";

    if (output_size == 0 || target.size() != output_size) {
        cout << "Invalid feature vec size:" << target.size() << ", required:" << output_size << endl;
        return pair<float, string> (0, best_match);
    }

    cv::Mat target_vec(static_cast<int>(target.size()), 1, CV_32F);
    for(unsigned int i = 0; i < target.size(); i++) {
        target_vec.at<float>(i) = target[i];
    }

    for (auto entry : *(data)){
        cv::Mat entry_vec(static_cast<int>(target.size()), 1, CV_32F);
        for(unsigned int i = 0; i < target.size(); i++) {
            entry_vec.at<float>(i) = entry.second[i];
        }

        dist = ComputeReidDistance(target_vec, entry_vec);
        if (dist < min_value){
            min_value = dist;
            best_match = entry.first;
        }
    }
    if (min_value >= g_threshold)
        best_match = "Unknown";

    return pair<float, string> (min_value, best_match);
}

threading_class::threading_class(){}

void threading_class::make_thread(){
    std::thread T=std::thread(&threading_class::threading_func, this);
    T.detach();
}

// Requires: Global variable FaceDetection, mGlob
// Returns: final faces ready for face recognition
vector<Rect> get_final_faces(){
    Mat frame;
    mtx.lock();
    frame = mGlob.clone();
    mtx.unlock();

    vector<Rect> final_faces; // Collection of faces ready for recognition
    final_faces.clear();

    FaceDetection.enqueue(frame);
    FaceDetection.submitRequest();
    FaceDetection.wait();
    FaceDetection.fetchResults();
    for (auto r : FaceDetection.results){
            if (r.confidence > 0.3) {
                final_faces.push_back(Rect(r.location.x , r.location.y, r.location.width , r.location.height));
            }
    }
    return final_faces;
}

void threading_class::threading_func(){
    cout << "Creating new thread for inferecne async" << endl;

    FaceDetection.initialize(face_detection_model_full_path_fp32, "GPU");
    FaceRecognition.initialize(face_recognition_model_full_path_fp32, "GPU");

    infer_width = FaceRecognition.get_input_width();
    output_size = FaceRecognition.get_output_size();

    data = read_text();

    while (true){
        if (need_process && infer_width != 0){
            mtx.lock();
            Mat frame= mGlob.clone();
            mtx.unlock();

            // Check if vector.txt has been updated by looking at ./modified.txt file.
            // if updated, reload vectors.txt
            string file = "./modified.txt";
            char buffer[1];
            int i = 0;
            std::fstream infile(file, ios::in|ios::out);
            if (infile && infile.is_open()) {
              infile.seekg(0);
              infile >> i;
              if (i == 1) {
                // File was updated.
                data = read_text();

              }
              infile.seekg(0);
              infile << 0;
              infile.close();
            }


            vector<Rect> final_faces=get_final_faces();

            //---------------------------face recognition----------------------------------
            set<string> recognized_person;
            vector<float> output_vector;
            pair<float, string> compare_result;

            // Considering face recog takes long, do it in this thread and copy to result to glob.
            vector<recog_result> local_recog_results;
            local_recog_results.clear();
            for (auto rect : final_faces){
                Mat cropped = get_cropped(frame, rect, infer_width , 2);

                FaceRecognition.load_frame(cropped);
                output_vector = FaceRecognition.do_infer();

                //-------------compare with the dataset, return the most possible person if any---
                compare_result = recognize(&data , output_vector);
                if (compare_result.second != "Unknown"
                    && recognized_person.find(compare_result.second) == recognized_person.end())   {
                    recognized_person.insert(compare_result.second);
                    recog_result a_result;
                    a_result.name = compare_result.second;
                    a_result.value = compare_result.first;
                    a_result.rectangle = rect;
                    local_recog_results.push_back(a_result);
                } else {
                    recognized_person.insert(compare_result.second);
                    recog_result a_result;
                    a_result.name = compare_result.second;
                    a_result.value = compare_result.first;
                    a_result.rectangle = rect;
                    local_recog_results.push_back(a_result);
                }
            }
            mtx.lock();
            global_recog_results.clear();
            for (auto result : local_recog_results){
                global_recog_results.push_back(result);
            }
            mtx.unlock();
        } else {
            usleep(5*1000);
        }
    }
}

MyPlugin::MyPlugin() {}


rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
    cout << "threshold: " << g_threshold << endl;

    threading_class t_c;
    t_c.make_thread();
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::PluginClose() {
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {
    // Fetch data from the frame and do face detection using the inference engine plugin
    // after update, send it back to analytics node.
    if (!buffer->buffer) {
        return RVA_ERR_OK;
    }
    if (buffer->width >= 320 && buffer->height >= 240) {

        cv::Mat mYUV(buffer->height + buffer->height / 2, buffer->width, CV_8UC1, buffer->buffer );
        cv::Mat mBGR(buffer->height, buffer->width, CV_8UC3);
        cv::cvtColor(mYUV,mBGR,cv::COLOR_YUV2BGR_I420);
        //--------------Update the mat for inference, and fetch the latest result  ------------------
        mtx.lock();
        if (mBGR.cols > 0 && mBGR.rows > 0){
            mGlob = mBGR.clone();
            if (!need_process) {
              need_process = true;
            }
        } else {
            cout << "One mBGR is not qualified for inference" << endl;
        }
        mtx.unlock();

        //-----------------------Draw the recongnition results-----------------------------------
        mtx.lock();
        for(auto result : global_recog_results){
            rectangle(mBGR,result.rectangle,Scalar(255, 255, 18), 2);
            putText(mBGR,
                        "Name : " + result.name ,
                        cv::Point2f(result.rectangle.x,result.rectangle.y - 20),
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(73, 255, 167), 1, CV_AA);
            putText(mBGR,
                        "Precision : " + to_string(result.value) ,
                        cv::Point2f(result.rectangle.x, result.rectangle.y - 3),
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(73, 255, 167), 1, CV_AA);
        }
        mtx.unlock();
        cv::cvtColor(mBGR,mYUV,cv::COLOR_BGR2YUV_I420);
        buffer->buffer = mYUV.data;
    }

    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }
    return RVA_ERR_OK;
}

// Declare the plugin
DECLARE_PLUGIN(MyPlugin)

