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
#include "face_detection.cpp"
#include "face_recognition.cpp"
//#include "person_detection.cpp"
#include "emotion_recognition.cpp"
#include "thread_func.h"
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

#include <common.hpp>
#include <slog.hpp>
#include "mkldnn/mkldnn_extension_ptr.hpp"
#include <ext_list.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <sys/stat.h>
#include <thread>
#include <mutex>

using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;

std::vector<std::pair<std::string, std::vector<float> > > data;
std::mutex mtx;

FaceDetectionClass FaceDetection;
FaceRecognitionClass FaceRecognition;
EmotionRecognitionClass EmotionRecognition;

cv::Mat mGlob(620,480,CV_8UC3);
vector<recog_result> global_recog_results;
volatile bool need_process=false;

/**returns: the cropped square image of crop_size having same center of rectangle**/
/**style=0 : original face detection results with largest possible square
   style>0 : larger square resized **/
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
            for (int j = 0; j < 512; ++j)
            {
                getline(infile , line);
                value=strtof(line.c_str(),0);
                vec.push_back(value);
            }
            data.push_back ( pair< string,vector<float> > (name, vec) );
        }
    }
    return data;
}

float calculate_dist(std::vector<float> v1, std::vector<float> v2, int len){
    float sum=0.0;
    float subtract=0.0;
    for (int i = 0; i < 512; ++i)
    {
        subtract=v1[i]-v2[i];
        sum=sum+abs(subtract*subtract);
    }
    return sqrt(sum);
}

//requires : data of name-feature set / person to be recognized /threshold 
pair<float , string> recognize(vector<pair<string, vector<float> > > *data , vector <float> target, float threshold){
    float dist=3.0;
    float min_value=3.0;
    string best_match="Unknown";
    for (auto entry : *(data)){
        dist=calculate_dist(target , entry.second , 512);
        if (dist < min_value){
            min_value=dist;
            best_match=entry.first;
        } 
    }
    if (min_value > threshold) best_match= "Unknown";
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
        x+=sub_width;
    }
    while (y < frame_height){
        vertical_bound.push_back(y);
        y+=sub_height;
    }
    //find the smallest square that emcompasses the rect, and then enlarge 9x
    //disregard out-of bound
    for (auto rr:pre_results){
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
                //cout<< "rect.y" <<r.y << "y_lim"<<y_lim<<"rect_all"<<rect.y+rect.height<<endl;
            } 
        }
        if (cross_boundary){
            need_redo.push_back(r);
        }
        else{
            bool problem = false; 
            
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
            if (problem) {cout<<"problem in pre"<<endl;}
            (*final_faces).push_back(rr);
        }
    }
    return need_redo;
}

void add_redo_rects_to_final(vector<Rect> *final_faces, vector<Rect> redo_rects, int sub_width, int sub_height, int frame_width,int frame_height){
    vector<Rect> enlarged_face_results;
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

    for (auto r:redo_rects){    
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
            (*final_faces).push_back(r);
        }
        else{
            
        }
    }
    return ;
}
   
threading_class::threading_class(){}

void threading_class::make_thread(){
    std::thread T=std::thread(&threading_class::threading_func, this);
    T.detach();
}

void sanity_check_final_face(vector<Rect> *final_faces, int frame_width , int frame_height){
    for (auto rect: *final_faces){
        if (    rect.x <0 || rect.y<0 || rect.x+rect.width> frame_width || rect.y+rect.height> frame_height
            )
        cout<< "problem! in sanity_check_final_face()"<<endl;
    }
    return;
}

//requires: face detection , a frame to be processed 
//return: all faces detected by frame-splitting method. 
//note: a face could be detected multiple times.
vector<Rect> get_final_faces(){
    Mat frame;
    mtx.lock();
    frame=mGlob.clone();
    mtx.unlock();
    //split incoming glob frame into smaller subs
    int sub_width;
    int sub_height;
    int sub_x , sub_y ;
    vector<cv::Rect> sub_frame_rects;
    vector<Rect> final_faces; //collection of faces ready for recognition
    final_faces.clear();
    int num_x =max(1,frame.cols/300);
    int num_y =max(1,frame.rows/300) ;
    for (int i = 0; i < num_x; ++i)
    {
        for (int j = 0; j < num_y; ++j)
        {
            sub_width = frame.cols/num_x;
            sub_height = frame.rows /num_y;
            sub_x = i * sub_width;
            sub_y = j * sub_height;
            if (i==num_x -1){
                sub_width = frame.cols - (num_x -1)*sub_width;
            }
            if (j==num_y -1){
                sub_height= frame.rows - (num_y -1)*sub_height;
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
            if (r.confidence>0.5) {
                detected_faces.push_back(Rect(r.location.x+rect.x , r.location.y + rect.y, r.location.width , r.location.height));
            }
        }
    }
    //some faces are at boundaries of each sub_frame, so need to count them in. 
    boundary_face_results=get_boundary_face_results(&final_faces,detected_faces , frame.cols/num_x, frame.rows/num_y,frame.cols, frame.rows);
    //for boundary faces, copy to a bigger mat, and do a face detection for them
    //in the boundary face restuls, form a good mat with all boundary faces, and then do facedetection once more. 
    //creata 300*300 square
    Mat boundary_face_frame = frame.clone();
    //white out the square
    boundary_face_frame= Scalar(255,255,255);
    resize(boundary_face_frame,boundary_face_frame,Size(300,300));

    //split into equal squares to store boundary faces
    int side_num = (int)sqrt(boundary_face_results.size())+1;
    int side_len = boundary_face_frame.cols / side_num ;
    int index=0;
    vector <pair<Rect, Rect>> links;  //keeps track of mapping from original frame to boundary_face_frame
    links.clear();
    vector <Rect> boundary_frame_squares; //collection of all nxn squares in the boundary_face_frame
    for (int i = 0; i < side_num; ++i)
    {
        for (int j = 0; j < side_num; ++j)
        {
            if (index==boundary_face_results.size())break;
            Rect src_rect= boundary_face_results[index];
            Rect dist_rect=Rect(i*side_len, 
                    j*side_len, 
                    side_len, 
                    side_len);
            boundary_frame_squares.push_back(dist_rect);
            Mat src=frame(src_rect).clone();
            Mat distROI=boundary_face_frame(dist_rect);
            resize(src,src, Size(side_len,side_len));
            src.copyTo(distROI);

            //first : original  second: square on boundary_frame
            links.push_back(pair<Rect,Rect>(src_rect,dist_rect));
            index++;
        }
    }

    FaceDetection.enqueue(boundary_face_frame);
    FaceDetection.submitRequest();
    FaceDetection.wait();
    FaceDetection.fetchResults();

    vector<Rect> redo_rects; //rectangles on original frame came from boundary-crossing faces
    for (auto result : FaceDetection.results){
            //cout << r.confidence <<endl;   
        if (result.confidence>0.3) {
            //rectangle(boundary_face_frame, result.location, Scalar(160,16,163));
        //put result back to original frame
            for (auto link : links){
                Rect boundary_square = link.second;
                Rect frame_square = link.first ;
                Rect r=result.location;
                if ( (boundary_square.x <= r.x) && 
                    (boundary_square.x+boundary_square.width >= r.x+r.width) &&
                    (boundary_square.y <= r.y) &&
                    (boundary_square.y+boundary_square.height >= r.y+r.height))
                {
                    //rectangle(boundary_face_frame, r, Scalar(160,16,163));

                    float ratio_wdith= frame_square.width / (float)boundary_square.width;
                    float ratio_height = frame_square.height / (float)boundary_square.height;
                    Rect actual_place;
                    actual_place.x = frame_square.x + (r.x - boundary_square.x)*ratio_wdith;
                    actual_place.y = frame_square.y + (r.y - boundary_square.y)*ratio_height;
                    actual_place.width= r.width * ratio_wdith;
                    actual_place.height =r.height * ratio_height ;
                    redo_rects.push_back(actual_place);
                    final_faces.push_back(actual_place);
                }    
            } 
        }
    }
    return final_faces;
}
//------------calling function for the new thread-------------------
void threading_class::threading_func(){
    std::cout<<"creating new thread for inferecne async"<<std::endl;
    while (true){
        if (need_process){
            mtx.lock();
            Mat frame= mGlob.clone();
            mtx.unlock();    
            //get all available faces for other detections
            vector<Rect> final_faces=get_final_faces();
    //----------------------------------------face recognition--------------------------------------
            set<string> recognized_person;
            vector<float> output_vector;
            vector<pair<string, float> > emotion_result; 
            pair<float, string> compare_result;
            //cout<< "final_faces size"<<final_faces.size()<<endl;
            //sanity_check_final_face(&final_faces,frame.cols,frame.rows);

            //considering face recog takes long, do it in this thread and copy to result to glob. 
            vector<recog_result> local_recog_results;
            local_recog_results.clear();
            for (auto rect : final_faces){
                Mat cropped=get_cropped(frame, rect, 160 , 2);
                EmotionRecognition.load_frame(cropped);
                emotion_result=EmotionRecognition.do_infer();
                FaceRecognition.load_frame(cropped);
                output_vector= FaceRecognition.do_infer();

                //-------------compare with the dataset, return the most possible person if any---
                compare_result=recognize(&data , output_vector , 0.65);
                if (compare_result.second!="Unknown" 
                    && recognized_person.find(compare_result.second)==recognized_person.end())   {
                    recognized_person.insert(compare_result.second);
                    recog_result a_result;
                    a_result.name=compare_result.second;
                    a_result.value=compare_result.first;
                    a_result.rectangle=rect;
                    //attach emotion
                    float prob_max=0.00;
                    for (auto entry: emotion_result){
                        if (prob_max <= entry.second){
                            prob_max = entry.second;
                        }
                    }
                    for (auto entry: emotion_result){
                        if (prob_max ==entry.second) {
                            a_result.emotion = entry.first;
                        } 
                    }
                    local_recog_results.push_back(a_result);
                }
                /*
                else {
                    recog_result b_result;
                    b_result.name="Unknown";
                    b_result.rectangle=rect;
                    b_result.value=-1.0;
                    local_recog_results.push_back(b_result);
                }
                */
            }
            mtx.lock();
            global_recog_results.clear();
            for (auto result : local_recog_results){
                global_recog_results.push_back(result);
            }   
            mtx.unlock();
            //cout<<"a frame"<<endl;
        }
        //else std::cout<<"no need"<<std::endl; 
    }
}


int main(){
//-----------------------Read data from "vectors.txt"----------------------
    data=read_text();

//-----------------------Initialize Detections -----------------------------
    FaceRecognition.initialize("./model/20180402-114759.xml");
    FaceDetection.initialize();
    EmotionRecognition.initialize("/opt/intel/computer_vision_sdk/deployment_tools/intel_models/emotions-recognition-retail-0003/FP32/emotions-recognition-retail-0003.xml");

    threading_class t_c;
    t_c.make_thread();

//----------------------------------Get video source -----------------------------
    VideoCapture cap(0); 
    cap.set(CV_CAP_PROP_FRAME_WIDTH,1920);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,1080);
          // Check if camera opened successfully
    if(!cap.isOpened()){
        cout << "Error opening video stream or file" << endl;
        return -1;
    }
    int fps=0;
//-------------------------------Another infer-----------------------------
    Mat mBGR;
    while (1){
        fps++;
        cout<<fps<<endl;
        cap.read(mBGR);

        if (mBGR.empty()){
            continue;
        }
// --------------------------split--face--recog----style ------------------
        mtx.lock();
        mGlob=mBGR.clone();
        need_process=true;
        mtx.unlock();

        mtx.lock();
        for(auto result : global_recog_results){
            if (result.name!= "Unknown") rectangle(mBGR,result.rectangle,Scalar(255,255,18),2);
            putText(mBGR, 
                        "Name : " + result.name , 
                        cv::Point2f(result.rectangle.x,result.rectangle.y -20), 
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(73,255,167), 1, CV_AA); 
            putText(mBGR, 
                        "Value : " + to_string(result.value) , 
                        cv::Point2f(result.rectangle.x, result.rectangle.y ), 
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(73,255,167), 1, CV_AA); 
            putText(mBGR, 
                        "Value : " + result.emotion , 
                        cv::Point2f(result.rectangle.x, result.rectangle.y +20), 
                        FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(73,255,167), 1, CV_AA);
        }
        mtx.unlock();
        imshow (" asdf", mBGR);
        waitKey(1);
    }
      return 0;
}
