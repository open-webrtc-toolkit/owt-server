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

#include "face_recognition.cpp"
#include <iostream>
#include <sys/types.h>
#include <string.h>
#include <fstream>
#include <map>

#include <sstream>

#include <common.hpp>

#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <sys/stat.h>

using namespace cv;


int main(){

    VideoCapture cap(0); 
        
          // Check if camera opened successfully
    if(!cap.isOpened()){
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    Mat frame, prev_frame , next_frame;
    char usr[50];
    string in;

    cout << "---Hit ESC to take a photo ----" <<endl;
    while(1){
     
        // Capture frame-by-frame
        cap.read(next_frame);
      
        // If the frame is empty, break immediately
        if (next_frame.empty())
          break;
        

        // Display the resulting frame
        if (!frame.empty())
        imshow( "Frame", frame );

        // Press  ESC on keyboard to exit
        char c=(char)waitKey(50);
        if(c>=27) {
            cout<< "saving photo "<<endl;
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
            auto str = oss.str();
            imwrite("./camera_photos/"+str+".jpg",prev_frame);
        }
        prev_frame = frame;
        frame = next_frame;
        next_frame = Mat();
    }
      
        // When everything done, release the video capture object
        cap.release();
         
        // Closes all the frames
        
    return 0;
}

