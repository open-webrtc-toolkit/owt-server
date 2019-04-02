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
#include <mutex>

#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"

#include "face_detection.h"
#include "face_recognition.h"
#include "person_detection.h"

#include "pluginconfig.h"
#include "myplugin.h"

using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;

std::vector<std::pair<std::string, std::vector<float> > > data;
std::mutex mtx;
FaceDetectionClass FaceDetection;
FaceRecognitionClass FaceRecognition;
PersonDetectionClass PersonDetection;
cv::Mat mGlob(1280,720, CV_8UC3);
vector<recog_result> global_recog_results;
float g_threshold = 0.45;
int infer_width = 0;
int infer_height = 0;
int output_size = 0;
volatile bool need_process=false;

float ComputeReidDistance(const cv::Mat& descr1, const cv::Mat& descr2) {
    float xy = descr1.dot(descr2);
    float xx = descr1.dot(descr1);
    float yy = descr2.dot(descr2);
    float norm = sqrt(xx * yy) + 1e-6;
    return 1.0f - xy / norm;
}

/**returns: the cropped square image of crop_size having same center of rectangle**/
/**style=0 : original face detection results with largest possible square
   style>0 : larger square resized **/
Mat get_cropped(Mat input, Rect rectangle , int crop_size , int style){
    int center_x= rectangle.x + rectangle.width / 2;
    int center_y= rectangle.y + rectangle.height / 2;

    int max_crop_size= min(rectangle.width , rectangle.height);

    int adjusted= max_crop_size * 3 / 2;

    std::vector<int> good_to_crop;
    good_to_crop.push_back(adjusted / 2);
    good_to_crop.push_back(input.size().height - center_y);
    good_to_crop.push_back(input.size().width - center_x);
    good_to_crop.push_back(center_x);
    good_to_crop.push_back(center_y);

    int final_crop= * (min_element(good_to_crop.begin(), good_to_crop.end()));

    Rect pre (center_x - max_crop_size /2, center_y - max_crop_size / 2 , max_crop_size , max_crop_size  );
    Rect pre2 (center_x - final_crop, center_y - final_crop , final_crop * 2  , final_crop * 2  );
    Mat r;
    if (style==0) {
        r = input (pre);
    } else {
        r = input (pre2);
    }

    resize(r,r,Size(crop_size,crop_size));
    return r;
}

// Read the vector saved by pre-process tool
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
        cout << " : " << count <<endl;
        for (int i = 0; i < count; ++i)
        {
            vec.clear();
            for (int j = 0; j < output_size; ++j)
            {
                getline(infile , line);
                value = strtof(line.c_str(),0);
                vec.push_back(value);
            }
            data.push_back ( pair< string,vector<float> > (name, vec) );
        }
    }
    return data;
}

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

//given: facedetection results that crosses sub-frame boundaries
//returns: boundary-corssing rectangles from input after enlarged to max. square, then 9x
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
        x += sub_width;
    }
    while (y < frame_height){
        vertical_bound.push_back(y);
        y += sub_height;
    }
    //find the smallest square that emcompasses the rect, and then enlarge 9x
    //disregard out-of bound
    for (auto rr:pre_results){
        Rect r(rr.x, rr.y, rr.width, rr.height);
        if (r.width > r.height){
            r.height = r.width;
        }
        else (r.width = r.height);
        //enlarge 3 times
        r.x = r.x - r.width;
        r.y = r.y - r.height ;
        r.width = r.width * 3;
        r.height = r.height *3 ;
        if (r.x < 0) r.x = 0;
        if (r.y < 0) r.y =0;
        if (r.x + r.width > frame_width) {
            r.width = frame_width - r.x ;
        }
        if (r.y + r.height > frame_height) {
            r.height = frame_height - r.y;
        }
        //now r is 9x as large
        //enlarged_face_results.push_back(r);
        bool cross_boundary = false;
        for (auto x_lim : horizontal_bound){
            if  ((r.x < x_lim) && (x_lim < r.x + r.width)){
                cross_boundary = true;
            }
        }

        for(auto y_lim : vertical_bound){
            if  ((r.y < y_lim) && (y_lim < r.y + r.height)){
                cross_boundary = true;
            }
        }
        if (cross_boundary){
            need_redo.push_back(r);
        }
        else{
            bool problem = false;

            if (rr.x+rr.width > frame_width){
                rr.width = frame_width - rr.x;
                problem = true;
            }
            if (rr.y+rr.height> frame_height){
                rr.height = frame_height - rr.y;
                problem = true;
            }
            if (rr.x < 0){
                problem = true;
                rr.x = 0;
            }
            if (rr.y < 0) {
                rr.y = 0;
                problem = true;
            }
            if (problem) {
                cout << "problem in pre" << endl;
            }
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


vector<Rect> get_final_faces(){
    Mat frame;
    mtx.lock();
    frame = mGlob.clone();
    mtx.unlock();
    //split incoming glob frame into smaller subs
    int sub_width;
    int sub_height;
    int sub_x , sub_y ;
    vector<cv::Rect> sub_frame_rects;
    vector<Rect> final_faces; //collection of faces ready for recognition
    final_faces.clear();
    int num_x = 2 ;
    int num_y = 2 ;
    for (int i = 0; i < num_x; ++i)
    {
        for (int j = 0; j < num_y; ++j)
        {
            sub_width = frame.cols/num_x;
            sub_height = frame.rows /num_y;
            sub_x = i * sub_width;
            sub_y = j * sub_height;
            if (i == num_x - 1){
                sub_width = frame.cols - (num_x - 1) * sub_width;
            }
            if (j == num_y - 1){
                sub_height = frame.rows - (num_y -1) * sub_height;
            }
            sub_frame_rects.push_back(Rect(sub_x ,sub_y, sub_width, sub_height));
        }
    }


    //for each sub_frame do facedetection, upon fetching results, check faces
    //near boudary, redo facedetection for these faces, and then for all
        //detected faces, do face recognition
    Mat sub_frame;
    vector <Rect> detected_faces , boundary_face_results;
    boundary_face_results.clear();
    for (auto rect : sub_frame_rects){
        sub_frame=frame(rect);
        FaceDetection.enqueue(sub_frame);
        FaceDetection.submitRequest();
        FaceDetection.wait();
        FaceDetection.fetchResults();

        for (auto r : FaceDetection.results){
            if (r.confidence > 0.3) {
                detected_faces.push_back(Rect(r.location.x+rect.x , r.location.y + rect.y, r.location.width , r.location.height));
            }
        }
    }
    //some faces are at boundaries of each sub_frame, so need to count them in.
    boundary_face_results = get_boundary_face_results(&final_faces, detected_faces , frame.cols/num_x, frame.rows/num_y,frame.cols, frame.rows);
    //for boundary faces, copy to a bigger mat, and do a face detection for them
    //in the boundary face restuls, form a good mat with all boundary faces, and then do facedetection once more.
    //creata 300*300 square
    Mat boundary_face_frame = frame.clone();
    //white out the square
    boundary_face_frame = Scalar(255,255,255);
    resize(boundary_face_frame,boundary_face_frame,Size(300,300));

    //split into equal squares to store boundary faces
    int side_num = (int)sqrt(boundary_face_results.size())+1;
    int side_len = boundary_face_frame.cols / side_num ;
    int index = 0;
    vector <pair<Rect, Rect>> links; // keeps track of mapping from original frame to boundary_face_frame
    links.clear();
    vector <Rect> boundary_frame_squares; //collection of all nxn squares in the boundary_face_frame
    for (int i = 0; i < side_num; ++i)
    {
        for (int j = 0; j < side_num; ++j)
        {
            if (index == boundary_face_results.size())break;
            Rect src_rect = boundary_face_results[index];
            Rect dist_rect = Rect(i*side_len,
                    j*side_len,
                    side_len,
                    side_len);
            boundary_frame_squares.push_back(dist_rect);
            Mat src = frame(src_rect).clone();
            Mat distROI = boundary_face_frame(dist_rect);
            resize(src,src, Size(side_len,side_len));
            src.copyTo(distROI);

            // first : original  second: square on boundary_frame
            links.push_back(pair<Rect,Rect>(src_rect,dist_rect));
            index++;
        }
    }

    FaceDetection.enqueue(boundary_face_frame);
    FaceDetection.submitRequest();
    FaceDetection.wait();
    FaceDetection.fetchResults();

    for (auto result : FaceDetection.results){
        if (result.confidence>0.3) {
            // Put result back to original frame
            for (auto link : links){
                Rect boundary_square = link.second;
                Rect frame_square = link.first ;
                Rect r = result.location;
                if ( (boundary_square.x <= r.x) &&
                    (boundary_square.x+boundary_square.width >= r.x+r.width) &&
                    (boundary_square.y <= r.y) &&
                    (boundary_square.y+boundary_square.height >= r.y+r.height))
                {

                    float ratio_wdith= frame_square.width / (float)boundary_square.width;
                    float ratio_height = frame_square.height / (float)boundary_square.height;
                    Rect actual_place;
                    actual_place.x = frame_square.x + (r.x - boundary_square.x)*ratio_wdith;
                    actual_place.y = frame_square.y + (r.y - boundary_square.y)*ratio_height;
                    actual_place.width = r.width * ratio_wdith;
                    actual_place.height = r.height * ratio_height ;
                    final_faces.push_back(actual_place);
                }
            }
        }
    }
    return final_faces;
}
//------------calling function for the new thread-------------------
void threading_class::threading_func(){
    FaceDetection.initialize(face_detection_model_full_path_fp32, "GPU");
    FaceRecognition.initialize(face_recognition_model_full_path_fp32, "GPU");

    PersonDetection.initialize("/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-person-detection-retail-0002/FP32/face-person-detection-retail-0002.xml");
    data=read_text();

    while (true){
        if (need_process){
            mtx.lock();
            Mat frame= mGlob.clone();
            mtx.unlock();
            vector<float> v;
            vector<cv::Rect> PersonDetection_rect;
            PersonDetection.load_frame(frame);
            v= PersonDetection.do_infer();

            int x,y,width,height;
            float confidence;
            PersonDetection_rect.clear();
            for (int i = 0; i < v.size(); i=i+7)
            {
                confidence= v[i+2];
                x=frame.cols*v[i+3];
                y=frame.rows*v[i+4];
                width= ( frame.cols * (v[i+5]-v[i+3]) );
                height=  (frame.rows * (v[i+6] - v[i+4]) );
                Rect rect (x ,y , width ,height);
                rect.height=rect.width;
                if (confidence > 0.2) {
                        PersonDetection_rect.push_back(rect);
                }
            }
            //end of persondetection

            Mat a_person;
            vector<Mat> persons;
            vector<Rect> FaceDetection_rect;

            vector<recog_result> local_recog_results;
            local_recog_results.clear();

            for (auto rect: PersonDetection_rect) {
                //cases when the output from persondetection is not good for cropping out the person
                if ((rect.width + rect.x) > frame.cols) {
                    rect.width = frame.cols- abs(rect.x);
                }
                if ((rect.height  + rect.y) > frame.rows) {
                    rect.height = frame.rows- abs(rect.y);
                }
                if (rect.x <0) {
                    rect.x=0;
                }
                if (rect.y <0){
                    rect.y=0;
                }
                //int overflow/bad output handeling
                if ( (abs(rect.x) > 10000 || abs(rect.y) > 10000 || abs(rect.height) > 10000 || abs(rect.width) > 10000) )
                    continue;
                a_person = frame(rect);
                FaceDetection.enqueue(a_person);
                FaceDetection.submitRequest();
                FaceDetection.wait();
                FaceDetection.fetchResults();

                //among all face detection results, do face recog.
                for (auto r : FaceDetection.results){
                    if (r.confidence > 0.2) {
                        std::vector<float> output_vector;
                        pair<float, string> compare_result;
                        //--------------for this frame, output the charactiristic vector ------------------
                        Mat cropped = get_cropped(a_person, r.location, 128 , 2);
                        FaceRecognition.load_frame(cropped);
                        output_vector= FaceRecognition.do_infer();
                        //-------------compare with the dataset, return the most possible person if any---
                        compare_result = recognize(&data , output_vector);

                        if (compare_result.second != "Unknown")   {
                            // The output position of facedetection is related
                            // to a_person's local corrdinate,
                            // so need to conver to global
                            recog_result a_result;
                            a_result.name = compare_result.second;
                            a_result.value = compare_result.first;
                            a_result.face_rect = Rect(r.location.x+rect.x, r.location.y+rect.y,
                                                    r.location.width,r.location.height);
                            a_result.body_rect = rect;
                            local_recog_results.push_back(a_result);
                        }
                    }
                }
            }

            mtx.lock();
            global_recog_results.clear();
            for (auto result : local_recog_results){
                global_recog_results.push_back(result);
            }
            mtx.unlock();
        }
    }
}

MyPlugin::MyPlugin(){}


rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
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
    if (buffer->width >= 320 && buffer->height >= 240) {

        cv::Mat mYUV(buffer->height + buffer->height / 2, buffer->width, CV_8UC1, buffer->buffer );
        cv::Mat mBGR(buffer->height, buffer->width, CV_8UC3);
        cv::cvtColor(mYUV,mBGR,cv::COLOR_YUV2BGR_I420);
        //--------------Update the mat for inference, and fetch the latest result  ------------------
        mtx.lock();
        if (mBGR.cols > 0 && mBGR.rows > 0){
            mGlob = mBGR.clone();
            need_process= true;
        } else {
            std::cout << "one mBGR is not qualified for inference" <<std::endl;
        }
        mtx.unlock();

        //-----------------------Draw the detection results-----------------------------------
        mtx.lock();
        for(auto result : global_recog_results){
            Rect background_rect(result.face_rect.x,
                    min(result.face_rect.y  + result.face_rect.height * 2, mBGR.rows - 50),
                    200, 60);
            rectangle(mBGR, background_rect, Scalar(216, 206, 199), -2);
            rectangle(mBGR, background_rect, Scalar(216, 206, 199), -2);
            putText(mBGR,
                    result.name ,
                    cv::Point2f(background_rect.x,background_rect.y+background_rect.height / 2),
                    FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(198, 76, 0), 1, CV_AA);
        }
        mtx.unlock();
        cv::cvtColor(mBGR,mYUV,cv::COLOR_BGR2YUV_I420);

        buffer->buffer=mYUV.data;
    }
    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }
    return RVA_ERR_OK;
}

// Declare the plugin
DECLARE_PLUGIN(MyPlugin)

