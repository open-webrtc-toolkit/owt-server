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
#include "person_detection.cpp"
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


using namespace InferenceEngine;
using namespace cv;
using namespace std;
using namespace InferenceEngine::details;


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
    string file="../vectors.txt";
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

pair<float , string> recognize(vector<pair<string, vector<float> > > *data , vector <float> target){
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
    if (min_value > 0.6) best_match= "Unknown";
    return pair<float, string> (min_value, best_match);
}


//given: complete faces, but maybe some rects crosses the sub frame boundaries
//return: all rects that crosses boundaries of original frame splitting
vector<Rect> get_boundary_faces(vector<Rect> final_face_results , int sub_width, int sub_height, int frame_width, int frame_height){
    vector<Rect> need_redo;
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

    for(auto rect:final_face_results){
        bool cross_boundary=false;
        for (auto x_lim : horizontal_bound){
            if  ((rect.x < x_lim) && (x_lim <rect.x+rect.width)){
                cross_boundary=true;
            }
        }

        for(auto y_lim : vertical_bound){
            if  ((rect.y < y_lim) && (y_lim<rect.y+rect.height)){
                cross_boundary=true;
                cout<< "rect.y" <<rect.y << "y_lim"<<y_lim<<"rect_all"<<rect.y+rect.height<<endl;
            } 
        }
        if (cross_boundary){
            need_redo.push_back(rect);
        }
    }
    return need_redo;
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
   

int main(){
//-----------------------Read data from "vectors.txt"----------------------
    vector<pair<string, vector<float> > > data=read_text();
    cout<< "Finished Reading"<<endl;
    FaceDetectionClass FaceDetection;
    FaceRecognitionClass FaceRecognition;
    PersonDetectionClass PersonDetection;

//-----------------------Initialize Detections -----------------------------
    FaceRecognition.initialize("../model/20180402-114759.xml");
    FaceDetection.initialize();
    PersonDetection.initialize("/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-person-detection-retail-0002/FP32/face-person-detection-retail-0002.xml");


//----------------------------------Get video source -----------------------------
    VideoCapture cap(0); 
        
          // Check if camera opened successfully
    if(!cap.isOpened()){
        cout << "Error opening video stream or file" << endl;
        return -1;
    }
    int fps=0;
//-------------------------------Another infer-----------------------------
    Mat frame;
    while (1){
        fps++;
        cout<<fps<<endl;
        cap.read(frame);

        if (frame.empty()){
            continue;
        }
// --------------------------split--face--recog----style ------------------
         //organize image splitting , assuming any aspect ratio
        int sub_width;
        int sub_height;
        int sub_x , sub_y ;
        vector<cv::Rect> sub_frame_rects;
        vector<Rect> final_faces; //collection of faces ready for recognition
        final_faces.clear();
        int num_x =3 ;
        int num_y =4 ;

        //the rightmost cols or botton rows may not have same cut as other subs
            //need to handle a little bit
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
        //pair of sub_frame - face_detection results
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
                //cout << r.confidence <<endl;
                
                if (r.confidence>0.3) {
                    detected_faces.push_back(Rect(r.location.x+rect.x , r.location.y + rect.y, r.location.width , r.location.height));
                }
            }
        }
        //some faces are at boundaries of each sub_frame, so need to count them in. 
        boundary_face_results=get_boundary_face_results(&final_faces,detected_faces , frame.cols/num_x, frame.rows/num_y,frame.cols, frame.rows);
        //for boundary faces, copy to a bigger mat, and do a face detection for them
        //in the boundary face restuls, form a good mat with all boundary faces, and then do facedetection once more. 
        //creata 300*300 frame 
        Mat boundary_face_frame = frame.clone();
        //white out the frame
        boundary_face_frame= Scalar(255,255,255);
        resize(boundary_face_frame,boundary_face_frame,Size(300,300));
        //split into equal squares to store boundary faces
        int side_num = (int)sqrt(boundary_face_results.size())+1;
        int side_len = boundary_face_frame.cols / side_num ;
        int index=0;
        vector <pair<Rect, Rect>> links;  //keeps track of mapping from original frame to boundary_face_frame
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
                        float ratio= frame_square.width / (float)boundary_square.width;
                        Rect actual_place;
                        actual_place.x = frame_square.x + (r.x - boundary_square.x)*ratio;
                        actual_place.y = frame_square.y + (r.y - boundary_square.y)*ratio;
                        actual_place.width= r.width * ratio;
                        actual_place.height =r.height * ratio ;
                        redo_rects.push_back(actual_place);
                        final_faces.push_back(actual_place);
                    }    
                } 
            }
        }

        //add_redo_rects_to_final(&final_faces, redo_rects, frame.cols/num_x, frame.rows/num_y, frame.cols, frame.rows);

        imshow("bf",boundary_face_frame);
        waitKey(1);

        //for (auto rect :detected_faces){
          //      rectangle(frame, rect,Scalar(155,125,73), 2);         
        //}

        for (auto rect : final_faces) {
           rectangle(frame, rect, cv::Scalar(73,255,255), 1) ;
        }    

        for (auto rect: sub_frame_rects) {
           rectangle(frame, rect, cv::Scalar(178,176,174), 1) ;
        }
//----------------------------------------face recognition--------------------------------------
        set<string> recognized_person;
        vector<float> output_vector;
        pair<float, string> compare_result;
        for (auto rect : final_faces){
            Mat cropped=get_cropped(frame, rect, 160 , 2);
            FaceRecognition.load_frame(cropped);
            output_vector= FaceRecognition.do_infer();
            //-------------compare with the dataset, return the most possible person if any---
            compare_result=recognize(&data , output_vector);
            if (compare_result.second!="Unknown" 
                && recognized_person.find(compare_result.second)==recognized_person.end())   {
                recognized_person.insert(compare_result.second);
                putText(frame, 
                    "Name : " + compare_result.second , 
                    cv::Point2f(rect.x,rect.y -15), 
                    FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(73,255,167), 1, CV_AA); 
                putText(frame, 
                    "Value : " + to_string(compare_result.first) , 
                    cv::Point2f(rect.x, rect.y ), 
                    FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(73,255,167), 1, CV_AA);  
            }
        }
        //cout<< "a frame" <<endl;
        imshow (" asdf", frame);
        waitKey(1);
    }
      return 0;
}
