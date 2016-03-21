/* <COPYRIGHT_TAG> */
#ifdef ENABLE_VA

#ifndef __FINDPLATE_H__
#define __FINDPLATE_H__
#include <opencv2/opencv.hpp>
#include <opencv/cv.hpp>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;

bool plate_recog(cv::Mat frame, cv::Rect in_rect, char* number);
#endif
#endif
