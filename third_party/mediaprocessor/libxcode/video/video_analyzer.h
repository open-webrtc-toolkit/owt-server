/* <COPYRIGHT_TAG> */
#ifdef ENABLE_VA

#ifndef __VA_SAMPLE_H__
#define __VA_SAMPLE_H__

#include <opencv2/opencv.hpp>
#include <opencv/cv.hpp>
#include <opencv2/core/ocl.hpp>
#include "mfxstructures.h"
#include "base/logger.h"

class OpencvVideoAnalyzer {
public:
	DECLARE_MLOGINSTANCE();
    OpencvVideoAnalyzer(bool display);
    ~OpencvVideoAnalyzer();

    bool GrayBackgroundSubstraction(cv::Mat &src, cv::Mat &background, cv::Mat &foreg);
    bool YUV2Gray(mfxFrameSurface1* surface, cv::Mat &mat);
    bool YUV2RGB(mfxFrameSurface1* surface, cv::Mat &mat);
    bool RGBA2RGB(mfxFrameSurface1* surface, cv::Mat &mat);
    bool RGB2YUV(cv::Mat &mat, mfxFrameSurface1* surface);
    int CarDetection(cv::Mat &img, cv::Rect in_rect, std::vector<cv::Rect> &rect_out);
    int PeopleDetection(cv::Mat &img, std::vector<cv::Rect> &rect);
    int ROIDetection(cv::Mat &image, std::vector<cv::Rect> &rect);
    int FaceDetection(cv::Mat& img, std::vector<cv::Rect> &rect);
    void MarkImage(cv::Mat &img, std::vector<cv::Rect> &rect, bool iscircle);
    void ShowImage(cv::Mat image);
    void DumpImage(cv::Mat image);
    int GetFeatures(cv::Mat &img, std::vector<cv::Rect> &rect, std::vector<float*> &f);

private:
    std::vector<int> DoMatch(std::vector<float*> &f, int, float* ref, int, float);
    int ExtractFeature(cv::Mat, float* v);
    double ComputeDistance(float* src, float* ref, int length);
    void filter_rects(const std::vector<cv::Rect>& candidates, std::vector<cv::Rect>& objects);
    std::vector<CvRect> combine_rect(CvSeq *contour, int threshold1, int threshold2,int threshold3,float beta);
    int UploadToSurface(cv::Mat &mat, mfxFrameSurface1 *surface_out);
    bool has_window;
    cv::Ptr<cv::BackgroundSubtractor> model;
};
#endif
#endif
