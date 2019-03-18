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


int main(){

    FaceDetectionClass FaceDetection;
    FaceDetection.initialize();
    FaceRecognitionClass FaceRecognition;


//------------------reading images of people---------------------------------
    string directory_path="./raw_photos";
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

//------------------processing/inference to crop image-----------------------------------------------
    cout<<endl<<"processing / output cropped image & vector txt"<<endl;
    Mat frame;
    Result the_result;
    string output_image_path;
    string output_path="./output_photos/";

    FaceRecognition.initialize("/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-reidentification-retail-0001/FP32/face-reidentification-retail-0001.xml");


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
        //cout<< info.first <<endl;
        //const int dir_err = mkdir((output_path + info.first).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        cout << "Making dir for: "<< (output_path + info.first).c_str();
        const int dir_err = mkdir((output_path + info.first).c_str(), S_IRWXU);
        if (-1 == dir_err)
        {
            cerr<<"Error creating directory! Either existing or no permission"<<endl;
            //exit(1);
        } 
        output_file << info.first << "\n" << info.second.size() << "\n";
        for (auto p : info.second){
            frame = imread (directory_path + "/" + info.first + "/" + p);
            FaceDetection.enqueue(frame);
            FaceDetection.submitRequest();
            FaceDetection.wait();
            FaceDetection.fetchResults(); 
            if (FaceDetection.results.size()>0){
                the_result=FaceDetection.results.front();
            }
            else continue;
            Mat cropped=get_cropped(frame, the_result.location, 128 , 2);
            output_image_path="./output_photos/" + info.first + "/" + fileNameNoExt(p) + "_cropped.jpg"  ;
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(100);
            imwrite( output_image_path, cropped, compression_params );

            FaceRecognition.load_frame(cropped);
            output_vector= FaceRecognition.do_infer();
          
            for (auto element : output_vector){
                  output_file<< setprecision(4) << element << "\n" ;
            }
        }
    }
    output_file.close();
    cout<< "finished cropping/saving photos" <<endl;

//-----------------output the vector into a .txt file for plugin to load/compare---------------------------
    cout<<endl<<"finished outputing to  .txt file"<<endl;

    return 0;
}
