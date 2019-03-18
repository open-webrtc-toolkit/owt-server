/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#include <unistd.h>
#include <iostream>
#include <string.h>
#include "myplugin.h"
#include <functional>
#include <fstream>
#include <random>
#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <map>
#include <inference_engine.hpp>

#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "plugin.h"
#include "face_detection.cpp"
#include "face_recognition.cpp"

#include <thread>
#include <mutex>
#include <ctime>

#include<stdlib.h>

using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;
std::vector<std::pair<std::string, std::vector<float> > > data;
std::mutex mtx;
FaceDetectionClass FaceDetection;
FaceRecognitionClass FaceRecognition;
cv::Mat mGlob(1280,720,CV_8UC3);
vector<recog_result> global_recog_results;
volatile bool need_process=false;

float g_threshold = 0.45;

#include <sys/time.h>
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

/**returns: the cropped square image of crop_size having same center of rectangle**/
/**style=0 : original face detection results with largest possible square
   style>0 : larger square resized(enlarged by 3/2) **/
Mat get_cropped(Mat input, Rect rectangle , int crop_size , int style){
    int center_x= rectangle.x + rectangle.width/2;
    int center_y= rectangle.y + rectangle.height/2;

    int max_crop_size= min(rectangle.width , rectangle.height);

    int adjusted= max_crop_size * 3 / 2;

    std::vector<int> good_to_crop;
    good_to_crop.push_back(adjusted / 2);
    good_to_crop.push_back(input.size().height - center_y);
    good_to_crop.push_back(input.size().width - center_x);
    good_to_crop.push_back(center_x);
    good_to_crop.push_back(center_y);

    int final_crop= * (min_element(good_to_crop.begin(), good_to_crop.end()));

    Rect pre (center_x - max_crop_size /2, center_y - max_crop_size /2 , max_crop_size , max_crop_size  );
    Rect pre2 (center_x - final_crop, center_y - final_crop , final_crop *2  , final_crop *2  );
    Mat r;
    if (style==0)   r = input (pre);
    else  r = input (pre2);

    resize(r,r,Size(crop_size,crop_size));
    return r;
}

//returns: data[]   person - 256D vector pair
vector<pair<string, vector<float> > > read_text(){
    cout<< "Reading vectors of saved person"<<endl;
    vector<pair<string, vector<float> > > data;
    string file="./vectors.txt";
    string line;
    std::ifstream infile(file);
    int count;
    float value ;
    vector <float> vec;
    string name;
    while (std::getline(infile, line))
    {
        name=string(line);
        cout<< name ;
        getline(infile , line);
        count= atoi(line.c_str());
        cout<<" : " << count <<endl;
        for (int i = 0; i < count; ++i)
        {
            vec.clear();
            for (int j = 0; j < 128; ++j)
            {
                getline(infile , line);
                value=strtof(line.c_str(),0);
                vec.push_back(value);
            }
            data.push_back ( pair< string,vector<float> > (name, vec) );
        }

        printf("+++database entry: %s\n", name.c_str());
        for (size_t i = 0; i < 16; i++) {
            for (size_t j = 0; j < 16; j++) {
                std::cout << "   " << fixed << setprecision(2) << vec[i * 16 + j];
            }
            std::cout << endl;
        }
        printf("---database entry: %s\n", name.c_str());
    }
    return data;
}

//requires: 2 256-D vectors
//returns : sqrt(L2) distance between vectors
float calculate_dist(std::vector<float> v1, std::vector<float> v2, int len){
    float sum=0.0;
    float subtract=0.0;
    for (int i = 0; i < 128; ++i)
    {
        subtract=v1[i]-v2[i];
        sum=sum+abs(subtract*subtract);
    }
    return sqrt(sum);
}

//requires: data and a person(target) to be recognized
//returns: distance and the name of the best-match person
//If distance is higher than the threshold, the name is "Unknown"
pair<float , string> recognize(vector<pair<string, vector<float> > > *data , vector <float> target){
#if 0
    float dist=3.0;
    float min_value=3.0;
    string best_match="Unknown";
    for (auto entry : *(data)){
        dist=calculate_dist(target , entry.second , 256);
        if (dist < min_value){
            min_value=dist;
            best_match=entry.first;
        }
    }
    if (min_value > 0.65) best_match= "Unknown";
    return pair<float, string> (min_value, best_match);
#else
    float dist = 0;
    float min_value = 100;
    string best_match="Unknown";

    if (target.size() != 128) {
        printf("Invalid feature vec size %ld, required %d\n", target.size(), 128);

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

        dist=ComputeReidDistance(target_vec, entry_vec);
        if (dist < min_value){
            min_value=dist;
            best_match=entry.first;
        }
    }
    if (min_value >= g_threshold) best_match= "Unknown";

    printf("distance %f, threshold %f, best_match %s\n", min_value, g_threshold, best_match.c_str());

    return pair<float, string> (min_value, best_match);
#endif
}

//Requires: collection of facedetection rectangles of each sub-frame
//Modifies: Final faces; add deteced faces ready for face-recognition
//Returns: 9X as large retangles of original input rectangles,
//        but only those who crosses boundaries of sub-frame
vector<Rect> get_boundary_face_results(vector<Rect> *final_faces,vector<Rect> pre_results , int sub_width, int sub_height, int frame_width,int frame_height){
    vector<Rect> enlarged_face_results;
    vector<Rect> need_redo;
    //deliminate bounds
    vector<int> horizontal_bound, vertical_bound;
    int x = sub_width;
    int y = sub_height;
    while (x < frame_width)
    {
        horizontal_bound.push_back(x);
        x+=sub_width;
    }
    while (y < frame_height){
        vertical_bound.push_back(y);
        y+=sub_height;
    }
    //find the smallest square that emcompasses the rect, and then enlarge 9x (3x3)
    //disregard out-of bound
    for (auto rr:pre_results){
        //make a copy of rr
        Rect r(rr.x, rr.y, rr.width, rr.height);
        if (r.width > r.height){
            r.height = r.width;
        }
        else (r.width=r.height);
        //enlarge 3 times
        r.x=r.x - r.width;
        r.y = r.y - r.height ;
        r.width = r.width * 3;
        r.height = r.height *3 ;
        if (r.x<0) r.x = 0;
        if (r.y<0) r.y =0;
        if (r.x + r.width > frame_width) {
            r.width = frame_width - r.x ;
        }
        if (r.y + r.height > frame_height) {
            r.height = frame_height - r.y;
        }
        //now r is 9x as large
        //enlarged_face_results.push_back(r);
        bool cross_boundary=false;
        for (auto x_lim : horizontal_bound){
            if  ((r.x < x_lim) && (x_lim <r.x+r.width)){
                cross_boundary=true;
            }
        }

        for(auto y_lim : vertical_bound){
            if  ((r.y < y_lim) && (y_lim<r.y+r.height)){
                cross_boundary=true;
            }
        }
        if (cross_boundary){
            need_redo.push_back(r);
        }
        else{
            bool problem = false;
            //small error handling cases. Please note that
            //the rectangle used is rr, the original one, not r, the squared one
            if (rr.x+rr.width>frame_width){
                rr.width=frame_width-rr.x;
                problem=true;
            }
            if (rr.y+rr.height> frame_height){
                rr.height=frame_height-rr.y;
                problem=true;
            }
            if (rr.x < 0){
                problem=true;
                rr.x=0;
            }
            if (rr.y < 0) {
                rr.y=0;
                problem=true;
            }
            //if (problem) {cout<<"problem in pre"<<endl;}
            (*final_faces).push_back(rr);
        }
    }
    return need_redo;
}


threading_class::threading_class(){}

void threading_class::make_thread(){
    std::thread T=std::thread(&threading_class::threading_func, this);
    T.detach();
}

//Requires: Global variable FaceDetection, mGlob
//Returns: final faces ready for face recognition
vector<Rect> get_final_faces(){
    Mat frame;
    mtx.lock();
    frame=mGlob.clone();
    mtx.unlock();

    vector<Rect> final_faces; //collection of faces ready for recognition
    final_faces.clear();

    FaceDetection.enqueue(frame);
    FaceDetection.submitRequest();
    FaceDetection.wait();
    FaceDetection.fetchResults();
    for (auto r : FaceDetection.results){
            if (r.confidence>0.3) {
                final_faces.push_back(Rect(r.location.x , r.location.y, r.location.width , r.location.height));
            }
    }
    return final_faces;
}
//------------calling function for the new thread-------------------
//Async inference function
void threading_class::threading_func(){
    std::cout<<"creating new thread for inferecne async"<<std::endl;

    FaceDetection.initialize();
    FaceRecognition.initialize("");

    while (true){
        if (need_process){
            mtx.lock();
            Mat frame= mGlob.clone();
            mtx.unlock();

            // Check if vector.txt has been updated by looking at ./modified.txt file.
            // if updated, reload vectors.txt
            string file="./modified.txt";
            char buffer[1];
            int i = 0;
            std::fstream infile(file, ios::in|ios::out);
            if (infile && infile.is_open()) {
              infile.seekg(0);
              infile >> i;
              if (i == 1) {
                // file was updated.
                data = read_text();

              }
              infile.seekg(0);
              infile << 0;
              infile.close();
            }


            vector<Rect> final_faces=get_final_faces();

    //----------------------------------------face recognition--------------------------------------
            set<string> recognized_person;
            vector<float> output_vector;
            pair<float, string> compare_result;
            //cout<< "final_faces size"<<final_faces.size()<<endl;

            //considering face recog takes long, do it in this thread and copy to result to glob.
            vector<recog_result> local_recog_results;
            local_recog_results.clear();
            for (auto rect : final_faces){
                Mat cropped=get_cropped(frame, rect, 128 , 2);

                FaceRecognition.load_frame(cropped);
                output_vector= FaceRecognition.do_infer();

                //-------------compare with the dataset, return the most possible person if any---
                compare_result=recognize(&data , output_vector);
                if (compare_result.second!="Unknown"
                    && recognized_person.find(compare_result.second)==recognized_person.end())   {
                    recognized_person.insert(compare_result.second);
                    recog_result a_result;
                    a_result.name=compare_result.second;
                    a_result.value=compare_result.first;
                    a_result.rectangle=rect;
                    local_recog_results.push_back(a_result);
                } else {
                    recognized_person.insert(compare_result.second);
                    recog_result a_result;
                    a_result.name=compare_result.second;
                    a_result.value=compare_result.first;
                    a_result.rectangle=rect;
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

MyPlugin::MyPlugin(){}


rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
    const char *env_hddl = "ANALYTICS_HDDL";
    const char *env_threshold = "ANALYTICS_RECOGNITION_THRESHOLD";
    char *p = NULL;

#if 0
    p = getenv(env_hddl);
    if (p) {
        printf("%s: %s\n", env_hddl, p);
    } else {
        printf("%s: %s\n", env_hddl, "NOT_SET");
    }
#endif

    p = getenv(env_threshold);
    if (p) {
        printf("%s: %s\n", env_threshold, p);
        g_threshold = atof(p);
    } else {
        printf("%s: %s\n", env_threshold, "NOT_SET");
    }

    printf("threshold: %f\n", g_threshold);

    data=read_text();
    cout<< "Making new thread for processing"<<endl;
    threading_class t_c;
    t_c.make_thread();
    cout<< "Successfully made new thread "<<endl;
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::PluginClose() {
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {
    // fetch data from the frame and do face detection using the inference engine plugin
    // after update, send it back to analytics server.
    if (!buffer->buffer) {
        return RVA_ERR_OK;
    }
    # if 1
    if (buffer->width >=320 && buffer->height >=240) {
        clock_t begin = clock();

        cv::Mat mYUV(buffer->height + buffer->height/2, buffer->width, CV_8UC1, buffer->buffer );
        cv::Mat mBGR(buffer->height, buffer->width, CV_8UC3);
        cv::cvtColor(mYUV,mBGR,cv::COLOR_YUV2BGR_I420);
        //--------------Update the mat for inference, and fetch the latest result  ------------------
        mtx.lock();
        if (mBGR.cols>0 && mBGR.rows>0){
            mGlob=mBGR.clone();
            if (!need_process) {
              need_process= true;
            }
        }
        else std::cout<<"one mBGR is not qualified for inference"<<std::endl;
        mtx.unlock();

        //-----------------------Draw the recongnition results-----------------------------------
        mtx.lock();
        for(auto result : global_recog_results){
            rectangle(mBGR,result.rectangle,Scalar(255,255,18),2);
            putText(mBGR,
                        "Name : " + result.name ,
                        cv::Point2f(result.rectangle.x,result.rectangle.y -20),
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(73,255,167), 1, CV_AA);
            putText(mBGR,
                        "Precision : " + to_string(result.value) ,
                        cv::Point2f(result.rectangle.x, result.rectangle.y -3),
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(73,255,167), 1, CV_AA);
        }
        mtx.unlock();
        cv::cvtColor(mBGR,mYUV,cv::COLOR_BGR2YUV_I420);
        //------------------------return the frame with
        buffer->buffer=mYUV.data;
        clock_t end = clock();
        //optional command for you to print out time for each frame
        //std::cout<<"time frame="<<double(end - begin) / CLOCKS_PER_SEC<<std::endl;
    }
    #endif
    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }
    return RVA_ERR_OK;
}

// Declare the plugin
DECLARE_PLUGIN(MyPlugin)

