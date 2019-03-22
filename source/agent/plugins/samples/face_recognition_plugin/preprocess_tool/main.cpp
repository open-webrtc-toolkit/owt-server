//Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
//
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
#include <sys/stat.h>
#include <thread>

#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "face_detection.h"
#include "face_recognition.h"
#include "config.h"

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


int main(){

    FaceDetectionClass FaceDetection;
    FaceDetection.initialize(face_detection_model_full_path_fp32, "GPU");
    FaceRecognitionClass FaceRecognition;


//------------------reading images of people---------------------------------
    string directory_path = "./raw_photos";
    std::map<string, vector <string>> photo_info;  //key: class name //value: image paths
    vector <string> input_names;
    cout << "Parsing Directory: " << directory_path << endl;
    DIR *dir;
    DIR *image_dir;
    struct dirent *entry;
    struct dirent *class_entry;
    string class_name;    
    string class_path;
    string file_name; 
    string file_path;
    if ((dir = opendir (directory_path.c_str())) != NULL) {
        while ((entry = readdir (dir)) != NULL) {
              if (entry->d_type == DT_DIR && strcmp (entry->d_name, ".") !=0 && strcmp (entry->d_name, "..") !=0) {
                  class_name = string (entry->d_name);
                  cout<< "-- " <<class_name <<endl;
                  class_path=directory_path + "/" + class_name;
                  if ( (image_dir = opendir (class_path.c_str())) != NULL) {
                      vector<string> images_in_class;
                      while ((class_entry = readdir (image_dir)) != NULL) {
                          if (class_entry->d_type != DT_DIR) {
                              file_name = string (class_entry->d_name);
                              file_path = directory_path + "/" + file_name;
                              cout <<"|----" <<file_name  << endl;
                              images_in_class.push_back(file_name);
                          }
                      }
                      photo_info.insert(pair<string, vector<string> > (class_name, images_in_class)  );
                  }
                  closedir(image_dir);
              }
          }
        closedir (dir);
    }

//------------------processing/inference to crop image----------------------------------------
    cout << "processing / output vector txt" << endl;
    Mat frame;
    Result the_result;

    FaceRecognition.initialize(face_recognition_model_full_path_fp32, "GPU");
    vector <float > output_vector;
    ofstream output_file; 
    output_file.open("./vectors.txt", std::ofstream::trunc);
    if (!output_file.is_open())
    {
        cout<< "unable to open vectors.txt"<<endl;
        return -1;
    }

    // first : name of the person
    //second : all paths to this person's photo 
    for (auto info : photo_info)
    {
        output_file << info.first << "\n" << info.second.size() << "\n";
        for (auto p : info.second){
            frame = imread (directory_path + "/" + info.first + "/" + p);
            FaceDetection.enqueue(frame);
            FaceDetection.submitRequest();
            FaceDetection.wait();
            FaceDetection.fetchResults(); 
            if (FaceDetection.results.size()>0){
                the_result=FaceDetection.results.front();
            } else continue;
            Mat cropped=get_cropped(frame, the_result.location, 128 , 2);

            FaceRecognition.load_frame(cropped);
            output_vector = FaceRecognition.do_infer();
          
            for (auto element : output_vector){
                  output_file<< setprecision(4) << element << "\n" ;
            }
        }
    }

//-----------------output the vector into a .txt file for plugin to load/compare--------------
    cout << "finished outputing to  .txt file" << endl;
    return 0;
}
