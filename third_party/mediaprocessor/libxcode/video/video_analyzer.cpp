/* <COPYRIGHT_TAG> */

#include "video_analyzer.h"

#ifdef ENABLE_VA

#include <vector>
#include <math.h>
#include <stdio.h>
#include "car_detector.h"


#define NUM_FEATURE_PER_OBJECT  6

using namespace cv;

DEFINE_MLOGINSTANCE_CLASS(OpencvVideoAnalyzer, "OpencvVideoAnalyzer");
OpencvVideoAnalyzer::OpencvVideoAnalyzer(bool display)
{
    model = createBackgroundSubtractorMOG2().dynamicCast<BackgroundSubtractor>();
    has_window = display;
    if (display) {
        namedWindow("video", WINDOW_AUTOSIZE);
        moveWindow("video", 0, 0);
    }
}

OpencvVideoAnalyzer::~OpencvVideoAnalyzer()
{
    if (has_window) {
        destroyWindow("video");
    }
}

void OpencvVideoAnalyzer::DumpImage(Mat image)
{
    char name[256] = {0};
    static int FrameIndex = 0;
    FrameIndex++;
    snprintf(name, sizeof(name), "frame_%d.jpg", FrameIndex);
    imwrite(name, image);
}

void OpencvVideoAnalyzer::ShowImage(Mat image)
{
    if (has_window) {
        imshow("video", image);
        waitKey(10);
    }
}
bool OpencvVideoAnalyzer::YUV2RGB( mfxFrameSurface1* surface, cv::Mat &mat )
{
    unsigned char* row;
    unsigned char* pY = surface->Data.Y;
    unsigned char* pU = surface->Data.U;

    unsigned char* ptr;

    int w, h;
    int Y,U,V,R,G,B;

    for(h=0;h<surface->Info.Height;h++)
    {
        row = mat.data + mat.step[0]*h;

        for(w=0;w<surface->Info.Width;w++)
        {
            Y = pY[h*surface->Data.Pitch+w];
            U = (pU[(h/2)*surface->Data.Pitch + (w/2)*2])-128;
            V = (pU[(h/2)*surface->Data.Pitch + (w/2)*2 + 1])-128;

            int rdif = V +((V*103)>>8);
            int invgdif = ((U*88)>>8) + ((V*183)>>8);
            int bdif = U + ((U*198)>>8);

            R = Y + rdif;
            G = Y - invgdif;
            B = Y + bdif;

            if( R>255 ) R=255;
            if( G>255 ) G=255;
            if( B>255 ) B=255;
            if( R<0 ) R=0;
            if( G<0 ) G=0;
            if( B<0 ) B=0;

            ptr = row + mat.step[1] * w;
            *(ptr) = B;
            *(ptr+1) = G;
            *(ptr+2) = R;
        }
    }

    return true;
}

bool OpencvVideoAnalyzer::GrayBackgroundSubstraction(cv::Mat &src, cv::Mat &background, cv::Mat &foreg)
{
    IplImage ImageSrc = src;
    IplImage ImageBackground = background;
    IplImage ImageForeground = foreg;

    cvAbsDiff(&ImageSrc,&ImageBackground, &ImageForeground);
    return true;
}

bool OpencvVideoAnalyzer::YUV2Gray(mfxFrameSurface1* surface, cv::Mat &mat)
{
    unsigned char* row;
    unsigned char* pY = surface->Data.Y;

    unsigned char* ptr;

    int w, h;
    int Y;
    for(h=0;h<surface->Info.Height;h++)
    {
        row = mat.data + mat.step[0]*h;

        for(w=0;w<surface->Info.Width;w++)
        {
            Y = pY[h*surface->Data.Pitch+w];
            ptr = row + mat.step[1] * w;
            *(ptr) = Y;
        }
    }

    return true;
}

bool OpencvVideoAnalyzer::RGBA2RGB(mfxFrameSurface1* surface, cv::Mat &mat)
{
    unsigned char* pB = surface->Data.B;
    unsigned int pitch = surface->Data.Pitch;

    unsigned char* dst_ptr;
    unsigned char* src_ptr;
    int w, h;

    for(h=0;h<surface->Info.Height;h++)
    {
        for(w=0;w<surface->Info.Width;w++)
        {
            src_ptr = pB + pitch * h + w * 4;
            dst_ptr = mat.data + mat.step[0] * h + mat.step[1] * w;
            memcpy(dst_ptr, src_ptr, 3);
        }
    }
    return true;
}

int OpencvVideoAnalyzer::UploadToSurface(cv::Mat &mat, mfxFrameSurface1* out_surf)
{
    unsigned char* y_src_ptr;
    unsigned char* u_src_ptr;
    unsigned char* v_src_ptr;
    unsigned char* dst_ptr;
    int pitch;
    int w;
    int h;
    pitch = out_surf->Data.Pitch;
    // Y plane
    y_src_ptr = mat.data;
    dst_ptr = out_surf->Data.Y;
    for (h = 0; h < out_surf->Info.Height; h++) {
        memcpy(dst_ptr, y_src_ptr, out_surf->Info.CropW);
        y_src_ptr = y_src_ptr + mat.step[0];
        dst_ptr = dst_ptr + pitch;
    }

    // UV plane
    dst_ptr = out_surf->Data.UV;
    u_src_ptr = mat.data + out_surf->Info.Width * out_surf->Info.Height;
    v_src_ptr = u_src_ptr + (out_surf->Info.Width / 2) * (out_surf->Info.Height / 2);
    for (h = 0; h < out_surf->Info.Height / 2; h++) {
        for (w = 0; w < out_surf->Info.Width / 2; w++) {
            *(dst_ptr + w * 2) = *(u_src_ptr + w);
            *(dst_ptr + w * 2 + 1) = *(v_src_ptr + w);
        }
        dst_ptr = dst_ptr + pitch;
        u_src_ptr = u_src_ptr + out_surf->Info.Width / 2;
        v_src_ptr = v_src_ptr + out_surf->Info.Width / 2;
    }

    return 0;
}

bool OpencvVideoAnalyzer::RGB2YUV(cv::Mat &mat, mfxFrameSurface1* surface)
{
    Mat yuvImg;
    cv::cvtColor(mat, yuvImg, CV_BGR2YUV_I420);
    UploadToSurface(yuvImg, surface);

    return true;
}

int OpencvVideoAnalyzer::ExtractFeature(cv::Mat roi, float* roi_vector)
{
    int sum_b, sum_g, sum_r;
    sum_b=sum_g=sum_r=0;
    int i, j, x, y;
    unsigned char* row;
    int ch = roi.channels();

    //get the upper body color at cols/2, rows/4
    x=(int)(roi.cols/2)-4;
    y=(int)(roi.rows/4)-4;

    for(i=0;i<8;i++)
    {
        row = roi.ptr<unsigned char>(y+i);
        for(j=0;j<8;j++)
        {
            sum_b += (int)row[x*ch+j*ch];
            sum_g += (int)row[x*ch+j*ch+1];
            sum_r += (int)row[x*ch+j*ch+2];
        }
    }

    roi_vector[0]=((float)sum_b/64);
    roi_vector[1]=((float)sum_g/64);
    roi_vector[2]=((float)sum_r/64);

    //get the lower part of the body color at cols/2, rows/4*3
    sum_b = 0;
    sum_g = 0;
    sum_r = 0;

    x=(int)(roi.cols/2)-4;
    y=(int)(roi.rows/4*3)-4;
    for(i=0;i<8;i++)
    {
        row = roi.ptr<unsigned char>(y+i);
        for(j=0;j<8;j++)
        {
            sum_b += (int)row[x*ch+j*ch];
            sum_g += (int)row[x*ch+j*ch+1];
            sum_r += (int)row[x*ch+j*ch+2];
        }
    }

    roi_vector[3]=((float)sum_b/64);
    roi_vector[4]=((float)sum_g/64);
    roi_vector[5]=((float)sum_r/64);

    return NUM_FEATURE_PER_OBJECT;
}

double OpencvVideoAnalyzer::ComputeDistance( float* src, float* ref, int length)
{
    int i;
    double sqre_sum = 0;
    double distance = 0.0;

    for(i=0;i<length;i++) {
        sqre_sum += pow(abs(src[i]-ref[i]),2);
    }

    distance = sqrt( sqre_sum );
    return distance;
}

void OpencvVideoAnalyzer::filter_rects(const std::vector<cv::Rect>& candidates, std::vector<cv::Rect>& objects)
{
    int i,j;

    for( i=0; i < candidates.size(); i++){
        cv::Rect rec = candidates[i];
        for( j=0;j<candidates.size();j++ )
            if(j!=i && (rec & candidates[j])==rec)
                break;
        if(j==candidates.size())
            objects.push_back(rec);
    }
}

int OpencvVideoAnalyzer::GetFeatures(cv::Mat &img, std::vector<cv::Rect> &rect, std::vector<float*> &f)
{
    int num_objects=0;
    float* features;
    features = (float*)malloc(rect.size()*sizeof(float)*NUM_FEATURE_PER_OBJECT);

    for(int i=0;i<rect.size();i++ ) {
        cv::Rect r = rect[i];
        cv::Mat roi(img, r );
        ExtractFeature(roi, &features[num_objects * NUM_FEATURE_PER_OBJECT]);
        f.push_back(&features[num_objects * NUM_FEATURE_PER_OBJECT]);
        num_objects++;
    }

    return num_objects;
}

std::vector<int> OpencvVideoAnalyzer::DoMatch(std::vector<float*> &features, int num_objects, float* ref, int feature_len, float threshold)
{
    int i;
    int objects;
    double distance;
    float* one_object;
    std::vector<int> matched;

    objects = num_objects<=features.size()? num_objects:features.size();
    for(i=0;i<objects;i++) {
        one_object = features[i];

        distance = ComputeDistance( one_object, ref, feature_len );
        if(distance<=threshold)
        {
            matched.push_back(i);
        }

    }
    features.clear();

    return matched;
}

void OpencvVideoAnalyzer::MarkImage(cv::Mat &img, std::vector<cv::Rect> &rect, bool iscircle)
{
    if (iscircle) {
        double scale = 1.3;
        for( std::vector<Rect>::const_iterator r = rect.begin(); r != rect.end(); r++) {
            Point center;
            int radius;

            double aspect_ratio = (double)r->width/r->height;
            if( 0.75 < aspect_ratio && aspect_ratio < 1.3 ) {
                center.x = cvRound((r->x + r->width*0.5)*scale);
                center.y = cvRound((r->y + r->height*0.5)*scale);
                radius = cvRound((r->width + r->height)*0.25*scale);
                circle(img, center, radius, cv::Scalar(0,255,0), 1);
            }
            else {
                rectangle(img, cvPoint(cvRound(r->x*scale), cvRound(r->y*scale)),
                           cvPoint(cvRound((r->x + r->width-1)*scale), cvRound((r->y + r->height-1)*scale)),
                           cv::Scalar(0,255,0), 1);
            }
        }
    } else {
        for (int i = 0; i < rect.size(); i++)
        {
            cv::Rect r_out = rect[i];
            cv::rectangle(img, r_out.tl(), r_out.br(), cv::Scalar(0,255,0), 1 );
        }
    }
}

int OpencvVideoAnalyzer::CarDetection(cv::Mat &img, cv::Rect in_rect, std::vector<cv::Rect>& rect_out)
{
    if(img.empty()||in_rect.width<100||in_rect.height<40)
        return 0;
    std::vector<float> x(detector, detector+sizeof(detector)/sizeof(float));

    cv::HOGDescriptor hog(cv::Size(40,40), cv::Size(10,10), cv::Size(5,5), cv::Size(5,5), 9);
    hog.setSVMDetector(x);

    cv::Mat image(img, in_rect);
    std::vector<Rect> f, found;

    hog.detectMultiScale(image, f, 0.6, cv::Size(5,5), cv::Size(10,10), 1.05, 4);
    filter_rects(f, found);
    if (found.size() > 0) {
        for (int i=0; i < found.size(); i++) {
            cv::Rect rt = found[i];
            if ((rt.x<0) || (rt.y<0) || (rt.width<0) || (rt.height<0)){
                continue;
            }
            if (((rt.x+rt.width)>image.cols) || ((rt.y+rt.height)>image.rows))
            {
                continue;
            }
            cv::Rect correct_rt((in_rect.x+rt.x), (in_rect.y+rt.y), rt.width, rt.height);
            rect_out.push_back(correct_rt);
        }
    }
    return rect_out.size();
}

int OpencvVideoAnalyzer::PeopleDetection(cv::Mat &img, std::vector<cv::Rect> &rect)
{
    UMat uimg;
    std::vector<cv::Rect> found;
    std::vector<cv::Rect> found_out;
    cv::HOGDescriptor hog;

    if(img.empty())
        return -1;
    uimg = img.getUMat(-1, USAGE_DEFAULT);
    hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
    hog.detectMultiScale(uimg, found, 0, cv::Size(8,8), cv::Size(0,0), 1.05, 2);
    filter_rects(found, found_out);
    for(int j=0;j<found_out.size();j++ )
    {
        cv::Rect rt = found_out[j];
        rt.x += cvRound(rt.width*0.1);
        rt.width = cvRound(rt.width*0.8);
        rt.y += cvRound(rt.height*0.07);
        rt.height = cvRound(rt.height*0.85);

        if(rt.x<0||rt.y<0||rt.width<0||rt.height<0)
        {
            continue;
        }
        if((rt.x+rt.width)>img.cols||(rt.y+rt.height)>img.rows)
        {
            continue;
        }

        rect.push_back(rt);
    }
    return rect.size();
}

std::vector<CvRect> OpencvVideoAnalyzer::combine_rect(CvSeq *contour, int threshold1, int threshold2, int threshold3, float beta)
{
    //Step 1: Get the rectangle.
    std::vector<CvRect> Rectlist;
    Rectlist.clear();

    std::vector<CvRect> newlist;
    newlist.clear();

    if (contour == NULL) {
        return newlist;
    }
    for (; contour != NULL; contour = contour->h_next) {
        CvRect rec = cvBoundingRect(contour, 0);
        Rectlist.push_back(rec);
    }

    //Step 2: Combine the rectangle together.
    CvRect rec;
    std::vector<CvRect>::iterator iter1=Rectlist.begin();

    for (; iter1 != Rectlist.end(); iter1++) {
        int newxmax,newxmin,newymax,newymin;
        float cenx,ceny,cenr;
        newxmax=iter1->x+iter1->width;
        newxmin=iter1->x;
        newymax=iter1->y+iter1->height;
        newymin=iter1->y;
        std::vector<CvRect>::iterator iter2;
        for(iter2=iter1+1;iter2 != Rectlist.end();) {
            if((iter2->width*iter2->height)>threshold2) {
                iter2++;
                continue;
            }

            float tempcenx,tempceny,tempcenr,diff;
            cenx=(newxmax+newxmin)/2;
            ceny=(newymax+newymin)/2;
            cenr=(sqrt((newxmax-newxmin)*(newxmax-newxmin)+(newymax-newymin)*(newymax-newymin)))/2;

            tempcenx=iter2->x+iter2->width/2;
            tempceny=iter2->y+iter2->height/2;
            tempcenr=(sqrt(iter2->height*iter2->height+iter2->width*iter2->width))/2;
            diff=sqrt((cenx-tempcenx)*(cenx-tempcenx)+(ceny-tempceny)*(ceny-tempceny));

            if(diff-cenr-tempcenr< threshold1) {
                if(iter2->x<newxmin) {
                    newxmin=iter2->x;
                }

                if(iter2->y<newymin) {
                    newymin=iter2->y;
                }

                if((iter2->x+iter2->width)>newxmax) {
                    newxmax=iter2->x+iter2->width;
                }

                if((iter2->y+iter2->height)>newymax) {
                    newymax=iter2->y+iter2->height;
                }
                iter2 = Rectlist.erase(iter2);
            } else {
                iter2++;
            }
        }
        rec.height =(int)(newymax -newymin)*beta;
        rec.width = (int)(newxmax -newxmin)*beta;
        rec.x =(int)((newxmin+newxmax)/2-rec.width/2);
        rec.y =(int)((newymin+newymax)/2-rec.height/2);
        if( rec.x<0 || rec.y<0 || rec.width<0 || rec.height<0 ) {
            continue;
        }
        newlist.push_back(rec);
    }

    std::vector<CvRect>::iterator iter3;
    for (iter3 = newlist.begin(); iter3 != newlist.end();) {
        if (iter3->width*iter3->height < threshold3) {
            iter3 = newlist.erase(iter3);
        } else {
            iter3++;
        }
    }
    return newlist;
}

int OpencvVideoAnalyzer::ROIDetection(cv::Mat& img, std::vector<cv::Rect> &rect)
{
    // Changed the foreImg to a local variable, in order to release it each time.
    cv::Mat foreImg;

    model->apply(img, foreImg, -1);
    IplImage fore = foreImg;

    cvErode(&fore, &fore, 0, 1);
    cvDilate(&fore, &fore, 0, 1);
    //find rounding box of fore-ground
    std::vector<CvRect> Rectlist;
    std::vector<CvRect>::iterator iter;
    int threshold1, threshold2, threshold3;

    CvMemStorage *men = NULL;
    CvSeq *contour = NULL;
    men = cvCreateMemStorage(0);
    cvFindContours(&fore, men, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    threshold1 = (img.cols) / 100;
    threshold2 = ((img.cols * img.rows) * 2) / 15;
    threshold3 = (img.cols * img.rows) / 30;
    Rectlist = combine_rect(contour, threshold1, threshold2, threshold3, 1.5);

    for (iter = Rectlist.begin(); iter != Rectlist.end(); iter++) {
        if( iter->x<0 || iter->y<0 || iter->width<0 || iter->height<0 ) {
            continue;
        }

        if( (iter->x+iter->width>img.cols) || (iter->y+iter->height>img.rows) ) {
            continue;
        }

        cv::Rect rec = *iter;
        rect.push_back(rec);
    }
    cvReleaseMemStorage(&men);
    return rect.size();
}

int OpencvVideoAnalyzer::FaceDetection(cv::Mat& img, std::vector<cv::Rect> &rect)
{
    std::string cascadeName;
    std::string nestedCascadeName;
    char* snva_home;
    snva_home = getenv("SNVA_HOME");
    if (snva_home == NULL) {
        cascadeName = "./haarcascade_frontalface_alt.xml";
        nestedCascadeName = "./haarcascade_eye_tree_eyeglasses.xml";
    } else {
        cascadeName= snva_home;
        cascadeName.append("/haarcascade_frontalface_alt.xml");
        nestedCascadeName = snva_home;
        nestedCascadeName.append("/haarcascade_eye_tree_eyeglasses.xml");
    }
    double scale = 1.3;
    bool tryflip = false;

    CascadeClassifier cascade;
    if( !cascade.load( cascadeName ) )
    {
        MLOG_ERROR(" Could not load classifier cascade\n");
        return -1;
    }

    CascadeClassifier nestedCascade;
    if( !nestedCascade.load( nestedCascadeName ) ) {
         MLOG_ERROR(" Could not load classifier cascade for nested objects\n");
    }

    Mat gray, smallImg( cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1);
    cvtColor(img, gray, CV_BGR2GRAY);
    resize(gray, smallImg, smallImg.size(), 0, 0, INTER_LINEAR);
    equalizeHist(smallImg, smallImg );

    cascade.detectMultiScale(smallImg, rect,
        1.1, 2, 0
        //|CV_HAAR_FIND_BIGGEST_OBJECT
        //|CV_HAAR_DO_ROUGH_SEARCH
        |CV_HAAR_SCALE_IMAGE
        ,
        Size(30, 30) );
    if(tryflip) {
        std::vector<Rect>faces2;
        flip(smallImg, smallImg, 1);
        cascade.detectMultiScale( smallImg, faces2,
                                 1.1, 2, 0
                                 //|CV_HAAR_FIND_BIGGEST_OBJECT
                                 //|CV_HAAR_DO_ROUGH_SEARCH
                                 |CV_HAAR_SCALE_IMAGE
                                 ,
                                 Size(30, 30) );
        for( std::vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end(); r++) {
            rect.push_back(Rect(smallImg.cols - r->x - r->width, r->y, r->width, r->height));
        }
    }

    return rect.size();
}

#endif
