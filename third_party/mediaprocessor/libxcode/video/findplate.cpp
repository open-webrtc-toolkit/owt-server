/* <COPYRIGHT_TAG> */

#include "findplate.h"

#ifdef ENABLE_VA

#include<cstdio>
#include<cstdlib>
#include<vector>
#include<string>
#include<math.h>

#define GrayScale 256
#define HIGH_WITH_CAR  4.8
#define CV_IMAGE_ELEM( image, elemtype, row, col )  (((elemtype*)((image)->imageData + (image)->widthStep*(row)))[(col)])
#define POINT_X  18
#define POINT_Y  2
#define CHARACTER 15
#define TEMPLETENUM  43
#define HORIZONTAL    1
#define VERTICAL    0

#define REGION_WIDTH    160
#define REGION_HEIGHT    75


typedef struct {
    int begin;
    int end;
    int dis;
}Letter;

typedef struct{
    int charnum;
    int matchnum;
    int whitematchnum;
}Chargrade;

char *PlateCode[TEMPLETENUM] =
{
    "0", "1", "2", "3", "4",
    "5", "6", "7", "8", "9",
    "A", "B", "C", "D", "E",
    "F", "G", "H", "J", "K",
    "L", "M", "N", "P", "Q",
    "R", "S", "T", "U", "V",
    "W", "X", "Y", "Z"
};
char *chinese[]=
{
    "京","津","沪","渝","冀","晋","辽","吉","黑","苏","浙","皖","闽","赣",
    "鲁","豫","鄂","湘","粤","琼","川","贵","云","陕","甘","青","藏","桂",
    "蒙","新","宁","港"
};

#define SAMPLE_COUNT (34)
#define SAMPLE_W (20)
#define SAMPLE_H (40)
// #define LOAD_SAMPLE
#ifdef LOAD_SAMPLE
  #ifndef MAKE_SAMPLE_H
    #define MAKE_SAMPLE_H
  #endif
  static uchar sample[SAMPLE_COUNT][SAMPLE_W*SAMPLE_H]={0};
  bool isloadsample=0;
  int loadsample(string filename);
#else
  #include<plateSample.h>
#endif /* LOAD_SAMPLE */

int otsu(const IplImage *frame);
CvRect findplate(IplImage *old);
CvRect refineplate(IplImage *tmpplate);
bool segplate(IplImage *complate, CvRect charrect[]);

char *Recogcharacter(IplImage *character, int type);
bool findplatenum(IplImage *pic,CvRect roi,char *number);
int isplateOK(CvRect tmprect,int w,int h);
void getbulewhitepoint(IplImage *tmpplate,int choose,CvPoint pointlist[]);
int isbule(int col,int row,CvPoint bule[],CvPoint white[],IplImage *src);
void prechar(IplImage * charplate ,CvRect charrect[],IplImage * charimg[]);
CvRect refinechar(IplImage *src);
//int train_ann(CvANN_MLP  &ann,string filename);
Mat getfeature(IplImage *onecharacter,int sizeData);
Mat ProjectedHistogram(Mat img, int t);
//char *classify(CvANN_MLP  &ann,Mat f);
int crossckeckchar(vector<Chargrade> gradelist,IplImage *charpic);

bool plate_recog(cv::Mat frame, cv::Rect in_rect, char* number)
{
    bool ret = true;
    IplImage pic = frame;
    cv::Rect rect(in_rect);
    ret = findplatenum(&pic, rect, number);
    if (ret) {
    }

    return ret;
}
/*int main()
{
    cv::Mat img = cv::imread("test\\test_image\\test_image\\BN3QSA_1224-480-309-309.jpg");
    char answer[10];
    struct roi myroi;
    cv::Rect rect;

    if(img.empty())
        return 0;

    myroi.x =0;
    myroi.y = 0;
    myroi.width = img.cols;
    myroi.height =img.rows;

    if (!plate_recog(img, myroi, answer))
        printf("no plate!");
    else
        printf("%s",answer);
        system("pause");

    return 0;
}*/
#ifdef LOAD_SAMPLE
int loadsample(string filename){
    string head;
    head.clear();
    head.reserve(50);
    head=filename+"/";

    if (isloadsample == 0)
    {
        for(int n=0;n<SAMPLE_COUNT;n++){
            string name;
            IplImage *num;
            IplImage *tmp;
            char c[16];
            snprintf(c, 16, "%d", n);
            name=head+c+".bmp";
            num = cvLoadImage(name.c_str(),CV_LOAD_IMAGE_UNCHANGED);
            if (num == NULL)
                return 0;
            tmp = cvCreateImage(cvSize(SAMPLE_W, SAMPLE_H), IPL_DEPTH_8U, 1);
            cvResize(num, tmp, CV_INTER_LINEAR);
            cvThreshold(tmp, tmp, 100,255, CV_THRESH_BINARY);
            memcpy(&sample[n][0], tmp->imageData, SAMPLE_W*SAMPLE_H);
            cvReleaseImage(&num);
            cvReleaseImage(&tmp);
        }
        isloadsample=1;

  #ifdef MAKE_SAMPLE_H
        // save to plateSample.h
        FILE *out = fopen("plateSample.h", "w");
        if (out == NULL) return 1;
        fprintf(out, "/* <COPYRIGHT_TAG> */\n\n");
        fprintf(out, "/* plateSample.h -- tables for plate samples\n");
        fprintf(out, " * Generated automatically by loadsample()@findplate.c\n */\n\n");

        fprintf(out, "const uchar sample[][SAMPLE_W*SAMPLE_H]={\n");
        for(int n=0;n<SAMPLE_COUNT;n++){
            fprintf(out, "  {\n");
            for(int row=0;row<SAMPLE_H;row++){
                fprintf(out, "    ");
                for(int col=0;col<SAMPLE_W;col++){
                    fprintf(out, "0x%02X,",sample[n][row*SAMPLE_W+col]);
                }
                fprintf(out, "\n");
            }
            fprintf(out, "  },\n");
        }
        fprintf(out, "};\n");
        fclose(out);
  #endif /* MAKE_SAMPLE_H */
    }

    return 1;
}
#endif /* LOAD_SAMPLE */

bool findplatenum(IplImage *pic,CvRect roi,char *number){
    bool ret=false;
    IplImage *src = NULL;
    IplImage *tmpplate = NULL;
    IplImage *platefinal = NULL;
    IplImage *complate = NULL;
    IplImage *charimg[7] = {NULL};

    CvMemStorage *men =cvCreateMemStorage(0);

    string platenum;
    CvRect charrect[7]={0};
    CvRect tmprec;
    CvRect rec_plate;
#ifdef LOAD_SAMPLE
    //loadsample
    if(isloadsample==0){
        if(!loadsample("/tmp/plate/sample/new")) {
            printf("Fail to Load the samples. Please put them under /tmp/plate/sample/new/\n");
            goto error;
        }
    }
#endif

    cvSetImageROI(pic,roi);
    src=cvCreateImage(cvSize(roi.width,roi.height),pic->depth,pic->nChannels);
    cvCopy(pic,src,NULL);
    cvResetImageROI(pic);

    rec_plate=findplate(src);
    if((rec_plate.height == 0)||(rec_plate.width == 0)) {
        // No plate found in image src.
        goto error;
    }

    tmpplate=cvCreateImage(cvSize(rec_plate.width,rec_plate.height),src->depth,src->nChannels);
    cvSetImageROI(src,rec_plate);
    cvCopy(src,tmpplate,NULL);
    cvResetImageROI(src);

    /*cvNamedWindow("tmpplate",CV_WINDOW_AUTOSIZE);
    cvShowImage("tmpplate",tmpplate);
    cvWaitKey();*/

    tmprec=refineplate(tmpplate);
    rec_plate.x=rec_plate.x+tmprec.x;
    rec_plate.y=rec_plate.y+tmprec.y;
    rec_plate.width=tmprec.width;
    rec_plate.height=tmprec.height;
    if(rec_plate.width<=0||rec_plate.height<=0) {
        // refine plate failed or it is not a plate.
        goto error;
    }

    cvSetImageROI(src,rec_plate);
    platefinal=cvCreateImage(cvSize(rec_plate.width,rec_plate.height),src->depth,src->nChannels);
    cvCopy(src,platefinal,NULL);
    cvResetImageROI(src);

    complate=cvCreateImage(cvSize(40*HIGH_WITH_CAR ,40),platefinal->depth,platefinal->nChannels);
    cvResize(platefinal,complate,CV_INTER_LINEAR);

    /*cvNamedWindow("complate",CV_WINDOW_AUTOSIZE);
      cvShowImage("complate",complate);
      cvWaitKey();*/

    for (int i = 0; i < 7;i++)
        charimg[i]=cvCreateImage(cvSize(20, 40), IPL_DEPTH_8U, 1);
    if (!segplate(complate, charrect)) {
        // Cutting character failure
        goto error;
    }
    prechar(complate ,charrect,charimg);

    /*cvNamedWindow("charimg",CV_WINDOW_AUTOSIZE);
    for(int i=0;i<7;i++){
       cvShowImage("charimg",charimg[i]);
       cvWaitKey();
    }*/

    platenum.clear();
    platenum.reserve(10);
    platenum=platenum+Recogcharacter(charimg[1], 1);
    platenum=platenum+Recogcharacter(charimg[2], 2);
    platenum=platenum+Recogcharacter(charimg[3], 2);
    platenum=platenum+Recogcharacter(charimg[4], 2);
    platenum=platenum+Recogcharacter(charimg[5], 0);
    platenum=platenum+Recogcharacter(charimg[6], 0);

    strncpy(number,platenum.c_str(), 6);
    ret = true;
error:
    // release all the reasorce here.
    if(src) cvReleaseImage(&src);
    if(tmpplate) cvReleaseImage(&tmpplate);
    if(platefinal) cvReleaseImage(&platefinal);
    if(complate) cvReleaseImage(&complate);
    for (int i = 0; i < 7;i++)
        if(charimg[i]) cvReleaseImage(&(charimg[i]));
    if(men) cvReleaseMemStorage(&men);

    return ret;
}

int otsu(const IplImage *frame)
{
    int width=frame->width;
    int height=frame->height;
    int pixelCount[GrayScale]={0};
    float pixelPro[GrayScale]={0};
    int i, j, pixelSum = width * height, threshold = 0;
    uchar* data = (uchar*)frame->imageData;
    float w0, w1, u0tmp, u1tmp, u0, u1, deltaTmp, deltaMax = 0;

    for(i = 0; i < height; i++)
    {
        for(j = 0;j < width;j++)
        {
            pixelCount[(int)data[i * width + j]]++;
        }
    }

    for(i = 0; i < GrayScale; i++)
    {
        pixelPro[i] = (float)pixelCount[i] / pixelSum;
    }

    for(i = 0; i < GrayScale; i++)
    {
        w0 = w1 = u0tmp = u1tmp = u0 = u1 = deltaTmp = 0;
        for(j = 0; j < GrayScale; j++)
        {
            if(j <= i)
            {
                w0 += pixelPro[j];
                u0tmp += j * pixelPro[j];
            }
            else
            {
                w1 += pixelPro[j];
                u1tmp += j * pixelPro[j];
            }
        }
        u0 = u0tmp / w0;
        u1 = u1tmp / w1;
        deltaTmp = (float)(w0 *w1* pow((u0 - u1), 2));
        if(deltaTmp > deltaMax)
        {
            deltaMax = deltaTmp;
            threshold = i;
        }
    }
    return threshold;
}

CvRect findplate(IplImage *src) {
    IplImage *gary;
    IplImage *sobel;
    IplImage *temp;
    IplImage *twoer;
    IplImage *open_close;
    CvMemStorage *men =cvCreateMemStorage(0);

    gary = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
    temp=cvCreateImage(cvGetSize(gary),IPL_DEPTH_16S, 1);
    sobel = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
    twoer = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
    open_close = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);

    cvCvtColor(src, gary, CV_BGR2GRAY);
    cvSmooth(gary,gary,CV_MEDIAN);
    cvSobel(gary,temp,2,0,3);

    cvReleaseImage(&gary);
    cvNormalize(temp,sobel,255,0,CV_MINMAX);

    cvThreshold(sobel, twoer, otsu(sobel),255, CV_THRESH_BINARY_INV);
    IplConvKernel *k1=cvCreateStructuringElementEx(3,1,1,0,CV_SHAPE_RECT);
    IplConvKernel *k2=cvCreateStructuringElementEx(3,3,1,0,CV_SHAPE_RECT);
    cvMorphologyEx(sobel,open_close,temp,k1,CV_MOP_CLOSE,13);
    cvMorphologyEx(open_close,open_close,temp,k2,CV_MOP_OPEN,1);
    cvReleaseImage(&sobel);
    cvReleaseStructuringElement(&k1);
    cvReleaseStructuringElement(&k2);

    cvThreshold(open_close, open_close, otsu(open_close),255, CV_THRESH_BINARY);
    cvReleaseImage(&temp);

    CvSeq *contours = NULL;
    CvScalar color = CV_RGB( 255, 0, 0);
    CvRect finalrect;
    int maxy=0;
    finalrect.height=0;finalrect.width=0;
    cvFindContours( open_close, men, &contours, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
    for( ; contours != 0; contours = contours->h_next)
    {
        CvRect aRect = cvBoundingRect( contours, 1 );
        if (isplateOK(aRect,src->width,src->height))
        {
            if(aRect.y>maxy){
                maxy=aRect.y;
                finalrect=aRect;
            }
        }
    }

    // To make sure that all the resources are released befor "return".
    cvReleaseImage(&open_close);
    cvReleaseImage(&twoer);
    cvReleaseMemStorage(&men);
    if(maxy==0)
        return finalrect;

    CvRect rec_plate;
    rec_plate.height=finalrect.height*1.3;
    rec_plate.width=finalrect.width*1.1;
    rec_plate.x=finalrect.x+finalrect.width/2-rec_plate.width/2;
    if(rec_plate.x<0)
        rec_plate.x=0;
    if(rec_plate.x+rec_plate.width>src->width)
        rec_plate.width=src->width-rec_plate.x;
        rec_plate.y=finalrect.y+finalrect.height/2-rec_plate.height/2;
        if(rec_plate.y<0)
            rec_plate.y=0;
        if(rec_plate.y+rec_plate.height>src->height)
            rec_plate.height=src->height-rec_plate.y;

    return rec_plate;
}

CvRect refineplate(IplImage *tmpplate){
    int r,g,b;
    CvRect rec_plate;
    IplImage *goodplate;
    CvPoint bule[4] = {0},white[4] = {0};

    getbulewhitepoint(tmpplate,2,bule);
    getbulewhitepoint(tmpplate,1,white);

    for(int row=0;row<tmpplate->height;row++){
        for(int col=0;col<tmpplate->width;col++){
            b = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 0];
            g = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 1];
            r = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 2];
            if(isbule(col,row,bule,white,tmpplate)){
                (tmpplate->imageData + row * tmpplate->widthStep)[col * tmpplate->nChannels + 0]=0;
                (tmpplate->imageData + row * tmpplate->widthStep)[col * tmpplate->nChannels + 1]=0;
                (tmpplate->imageData + row * tmpplate->widthStep)[col * tmpplate->nChannels + 2]=0;
            } else {
                (tmpplate->imageData + row * tmpplate->widthStep)[col * tmpplate->nChannels + 0]=255;
                (tmpplate->imageData + row * tmpplate->widthStep)[col * tmpplate->nChannels + 1]=255;
                (tmpplate->imageData + row * tmpplate->widthStep)[col * tmpplate->nChannels + 2]=255;
            }
        }
    }

    goodplate=cvCreateImage(cvGetSize(tmpplate),IPL_DEPTH_8U, 1);
    cvCvtColor(tmpplate, goodplate, CV_BGR2GRAY);
    //cvSmooth(goodplate, goodplate, CV_BLUR, 3, 3);
    //cvThreshold(goodplate, goodplate, otsu(goodplate), 255, CV_THRESH_BINARY);

    int ymax=goodplate->height,ymin=0,xmax=goodplate->width,xmin=0;
    int count = 0, pixelColor;
    for(int col=0;col<goodplate->width;col++){
        count=0;
        for(int row=0;row<goodplate->height;row++){
            pixelColor=((uchar *)(goodplate->imageData + row * goodplate->widthStep))[col];
            if(pixelColor==0)
                count++;
        }
        if(count>(float)(goodplate->height*0.6)){
            xmin=col;
            break;
        }
    }

    for(int col=goodplate->width-1;col>=0;col--){
        count=0;
        for(int row=0;row<goodplate->height;row++){
            pixelColor=((uchar *)(goodplate->imageData + row * goodplate->widthStep))[col];
            if(pixelColor==0)
                count++;
        }

        if(count>(float)(goodplate->height*0.6)){
            xmax=col;
            break;
        }
    }

    int row;

    for (row = int(goodplate->height/2); row>=0; row--){
        count=0;
        for (int col=4; col <=goodplate->width-5; col++){
            if (CV_IMAGE_ELEM(goodplate, uchar, row, col) != CV_IMAGE_ELEM(goodplate, uchar, row, col+1))
                count++;
        }
        if(count<10){
            ymin=row;
            if(ymin<(int(goodplate->height/2)-int(goodplate->height/4)))
                break;
        }
    }
    if(count>=7)
        ymin=row;
    for (row = int(goodplate->height / 2); row <goodplate->height; row++){
        count=0;
        for (int col=4; col <=goodplate->width-5; col++){
            if (CV_IMAGE_ELEM(goodplate, uchar, row, col) != CV_IMAGE_ELEM(goodplate, uchar, row, col+1))
                count++;
        }

        if(count<7){
            ymax=row;
            if(ymax>(int(goodplate->height/2)+int(goodplate->height/4)))
                break;
        }
    }
    if(count>=7) ymax=row;

    cvReleaseImage(&goodplate);
    rec_plate.x=xmin;
    rec_plate.y=ymin;
    rec_plate.width=xmax-xmin;
    rec_plate.height=ymax-ymin;
    return rec_plate;
}

bool segplate(IplImage *complate,CvRect charrect[]){
    IplImage *seplate,*coplate,*plate;
    CvPoint bule[4],white[4];
    int b,g,r;

    coplate=cvCreateImage(cvGetSize(complate),complate->depth, complate->nChannels);
    seplate=cvCreateImage(cvGetSize(complate),IPL_DEPTH_8U, 1);

    cvCopy(complate,coplate,NULL);

    getbulewhitepoint(coplate,2,bule);
    getbulewhitepoint(coplate,1,white);

    for(int row=0;row<coplate->height;row++){
        for(int col=0;col<coplate->width;col++){
            b = ((uchar *)(coplate->imageData + row * coplate->widthStep))[col * coplate->nChannels + 0];
            g = ((uchar *)(coplate->imageData + row * coplate->widthStep))[col * coplate->nChannels + 1];
            r = ((uchar *)(coplate->imageData + row * coplate->widthStep))[col * coplate->nChannels + 2];
            if(isbule(col,row,bule,white,coplate)){
                (seplate->imageData + row * seplate->widthStep)[col * seplate->nChannels + 0]=0;
            }
            else
            {
                (seplate->imageData + row * seplate->widthStep)[col * seplate->nChannels + 0]=255;
            }
        }
    }

    plate=cvCreateImage(cvGetSize(complate),IPL_DEPTH_8U, 1);
    cvCvtColor(complate, plate, CV_BGR2GRAY);
    cvThreshold(plate, plate, otsu(plate), 255, CV_THRESH_BINARY);
    cvSmooth(plate, plate, CV_BLUR, 3, 3);
    cvThreshold(plate, plate, otsu(plate), 255, CV_THRESH_BINARY);

    int num_h_se[192]={0};
    int num_h_p[192] = { 0 };
    int letter[14] = {0};
    std::vector<Letter> v_letter;
    for (int i = 0,j=0; i<40 * HIGH_WITH_CAR; i++)
    {
        num_h_se[i] = 0;
        num_h_p[i] = 0;
        for (j = 0; j<19; j++)
        {
            num_h_se[i] += CV_IMAGE_ELEM(seplate, uchar, j, i) / 254;
            num_h_p[i] +=  CV_IMAGE_ELEM(plate, uchar, j, i) / 254;
        }
        for (j = 21; j<40; j++)
        {
            num_h_se[i] += CV_IMAGE_ELEM(seplate, uchar, j, i) / 254;
            num_h_p[i] +=  CV_IMAGE_ELEM(plate, uchar, j, i) / 254;
        }
    }

    int one,two;
    v_letter.clear();

    for (int t = 3; t<40 * HIGH_WITH_CAR-4; t++){
        while(t<40 * HIGH_WITH_CAR&&num_h_se[t]<2) t++;
        one=t;
        while(t<40 * HIGH_WITH_CAR&&num_h_se[t]>2) t++;
        two=t;
        Letter oneletter;
        oneletter.begin = one;
        oneletter.end = two;
        oneletter.dis =abs( int((one + two) / 2) - int(plate->width / 2));
        if ((oneletter.end-oneletter.begin)>1)  v_letter.push_back(oneletter);
            t++;
    }

    int change = 0;
    for (std::vector<Letter>::iterator iter = v_letter.begin(); iter != v_letter.end(); iter++){
        change = 0;
        if ((iter->end - iter->begin) < 10&&(iter->dis!=plate->width)){
            if (change==0&&(iter + 1) != v_letter.end()){
                std::vector<Letter>::iterator tempiter = iter + 1;
                if ( abs(tempiter->end - iter->begin)<=20){
                    int collift,colright;
                    for(collift=iter->end;collift<=tempiter->begin;collift++){
                        int count=0;
                        for(int row=0;row<plate->height;row++){
                        if(CV_IMAGE_ELEM(plate, uchar, row, collift)==255)
                            count++;
                        }
                        if(count<2)
                            break;
                    }
                    for(colright=tempiter->begin;colright>=iter->end;colright--){
                        int count=0;
                        for(int row=0;row<plate->height;row++){
                            if(CV_IMAGE_ELEM(plate, uchar, row, colright)==255)
                                count++;
                        }
                        if(count<2)
                            break;
    	            }

                    if((colright-collift)<3){
                        change = 1;
                        iter->end = tempiter->end;
                        iter->dis = abs(int((iter->begin + iter->end) / 2) - int(plate->width / 2));
                        tempiter->dis = plate->width;
                    }
                }
            }
            if (change!=1){
                int new_one = 0, new_two =iter->end;
                for (int i = 0; i < 5; i++){
                    if ((iter->begin - i)>0 && num_h_p[iter->begin - i]>2)
                        new_one = iter->begin - i;
                        if ((iter->end + i)<plate->width&&num_h_p[iter->end + i]>2)
                            new_two = iter->end + i;
                }
                if(new_one!=0||new_two !=iter->end){
                    iter->begin = new_one;
                    iter->end = new_two;
                    iter->dis = abs(int((new_one + new_two) / 2) - int(plate->width / 2));
                }
            }
        }
    }

    vector<Letter>::iterator disiter=v_letter.begin();
    while(disiter!=v_letter.end()){
        if(disiter->dis==plate->width)
            disiter=v_letter.erase(disiter);
        else
            disiter++;
    }

    std::vector<Letter> addtions;
    addtions.clear();
    for (std::vector<Letter>::iterator iter = v_letter.begin(); iter !=v_letter.end(); iter++){
        if ((iter->end - iter->begin) > 30){
            Letter addletter;
            int mid = int((iter->begin + iter->end) / 2);
            addletter.begin = mid;
            addletter.end = iter->end;
            addletter.dis = abs(int((addletter.begin +addletter.end) / 2) - int(plate->width / 2));
            iter->end = mid;
            iter->dis = abs(int((iter->begin + iter->end) / 2) - int(plate->width / 2));
            addtions.push_back(addletter);
        }
    }

    for (std::vector<Letter>::iterator iter = addtions.begin(); iter !=addtions.end(); iter++)  v_letter.push_back(*(iter));
        Letter templetter;
        //第一个字符是汉字不可能太窄
        if(v_letter.size()>7){
            int first=0,erasesize=v_letter.size();
            for(int i=0;i<v_letter.size();i++){
                if((v_letter[i].end-v_letter[i].begin)<10)
                    first=i+1;
                else
                    break;
            }
            vector<Letter>::iterator eraseiter=v_letter.begin();
            for(int i=0;i<first&&i<(erasesize-7);i++){
                eraseiter=v_letter.erase(eraseiter);
            }
        }

        if (v_letter.size() > 7){
        //重新计算中心和句中的距离dis
        int remid,quo;
        quo=v_letter.size()/2;
        if((v_letter.size()%2)==0){
            remid=int((v_letter[quo-1].end+v_letter[quo-2].begin)/2);
        }
        else
            remid=int((v_letter[quo].end+v_letter[quo-2].begin)/2);

        for(int i=0;i<v_letter.size();i++){
            int onemid=int((v_letter[i].end+v_letter[i].begin)/2);
            v_letter[i].dis=abs(onemid - remid);
        }
        for (int i = 0; i < v_letter.size() - 7; i++){
            for (int j = 0; j < v_letter.size() - 1 - i; j++){
                if (v_letter[j].dis > v_letter[j + 1].dis){
                    templetter = v_letter[j + 1];
                    v_letter[j + 1] = v_letter[j];
                    v_letter[j] = templetter;
                }
            }
        }
        while (v_letter.size()>7) v_letter.pop_back();
    }

    cvReleaseImage(&coplate);
    cvReleaseImage(&seplate);
    cvReleaseImage(&plate);
    if(v_letter.size()<7)
        return 0;

    for (int i = 0; i < v_letter.size() - 1; i++){
        for (int j = 0; j < v_letter.size() - 1 - i; j++){
            if (v_letter[j].begin>v_letter[j + 1].begin){
                templetter = v_letter[j + 1];
                v_letter[j + 1] = v_letter[j];
                v_letter[j] = templetter;
            }
        }
    }

    for (int i = 0; i < v_letter.size(); i++){
        letter[2 * i] = v_letter[i].begin;
        letter[2 * i + 1] = v_letter[i].end;
    }
    for(int i=0;i<7;i++){
        charrect[i].x=letter[2*i];
        charrect[i].width =letter[2*i+1]-letter[2*i];
        charrect[i].y = 0;
        charrect[i].height = 40;
    }

    return 1;
}

char *Recogcharacter(IplImage *character,int type){
    int  char_start = 0, char_end = 0;
    int matchnum=0,whitematchnum=0;

    string head;
    head.clear();
    switch(type){
     case 1:
        char_start=10;char_end=33;
        break;
     case 2:
        char_start=0;char_end=33;
        break;
     case 0:
        char_start=0;char_end=9;
        break;
     case 3:
        char_start=1;char_end=32;
        break;
    }
    vector<Chargrade> allgrade;
    allgrade.clear();
    allgrade.reserve(40);

    for (int n = char_start; n <= char_end; n++){
        matchnum = 0;
        whitematchnum = 0;
        for(int row=0;row<SAMPLE_H;row++){
            for(int col=0;col<SAMPLE_W;col++){
                if(sample[n][row*SAMPLE_W + col]==CV_IMAGE_ELEM(character, uchar, row, col)){
                    if(sample[n][row*SAMPLE_W + col]==255)
                        whitematchnum++;
                    matchnum++;
                }
            }
        }

        Chargrade onegrade;
        onegrade.charnum=n; onegrade.matchnum=matchnum;
        onegrade.whitematchnum=whitematchnum;
        allgrade.push_back(onegrade);
    }
    Chargrade tmpgrade;
    for (int i = 0; i < allgrade.size() - 1; i++){
        for (int j =  allgrade.size()-1; j >i ; j--){
            if (allgrade[j].matchnum>allgrade[j-1].matchnum){
                tmpgrade=allgrade[j-1];
                allgrade[j-1]=allgrade[j];
                allgrade[j]=tmpgrade;
            }
        }
    }

    if(type==2||type==0)
        return PlateCode[crossckeckchar(allgrade,character)];
    else
        return PlateCode[allgrade[0].charnum];
}

int isplateOK(CvRect tmprect,int w,int h){
    float ratio;
    int xmid;//,ymid;
    ratio=float(float(tmprect.width)/float(tmprect.height));
    xmid=tmprect.x+int(tmprect.width/2);
    if(tmprect.y<(h/2))
        return 0;
        //X范围限定修改
    if(abs(xmid-(w/2))>tmprect.width)
        return 0;
    if(ratio>7||ratio<2.6)
        return 0;
    if(tmprect.height*tmprect.width<100)
        return 0;
    return 1;
}

void getbulewhitepoint(IplImage *tmpplate,int choose,CvPoint pointlist[]){
    CvPoint point;
    point.x=0;point.y=0;
    int g,r,b;
    int tar = 0,tmptar = 0;
    if(choose==1)
        tar=0;
    else
        tar=-1000;
    for(int col=int(tmpplate->width/2-tmpplate->width/4);col<int(tmpplate->width/2);col++){
        for(int row=int(tmpplate->height/4);row<int(tmpplate->height/2);row++){
            b = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 0];
            g = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 1];
            r = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 2];
            if(choose==1)
                tmptar=b+g+r;
            else
                tmptar=b-g-r;
            if(tmptar>tar){
                tar=tmptar;
                pointlist[0].x=row;
                pointlist[0].y=col;
            }
        }
    }

    if(choose==1)
        tar=0;
    else
        tar=-1000;

    for(int col=int(tmpplate->width/2);col<int(tmpplate->width/2+tmpplate->width/4);col++){
        for(int row=int(tmpplate->height/4);row<int(tmpplate->height/2);row++){
            b = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 0];
            g = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 1];
            r = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 2];
            if(choose==1)
                tmptar=b+g+r;
            else
                tmptar=b-g-r;
            if(tmptar>tar){
                tar=tmptar;
                pointlist[1].x=row;
                pointlist[1].y=col;
            }
        }
    }

    if(choose==1)
        tar=0;
    else
        tar=-1000;
    for(int col=int(tmpplate->width/2-tmpplate->width/4);col<int(tmpplate->width/2);col++){
        for(int row=int(tmpplate->height/2);row<int(tmpplate->height/2+tmpplate->height/4);row++){
            b = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 0];
            g = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 1];
            r = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 2];
            if(choose==1)
                tmptar=b+g+r;
            else
                tmptar=b-g-r;
            if(tmptar>tar){
                tar=tmptar;
                pointlist[2].x=row;
                pointlist[2].y=col;
            }
        }
    }

    if(choose==1)
        tar=0;
    else
        tar=-1000;

    for(int col=int(tmpplate->width/2);col<int(tmpplate->width/2+tmpplate->width/4);col++){
        for(int row=int(tmpplate->height/2);row<int(tmpplate->height/2+tmpplate->height/4);row++){
            b = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 0];
            g = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 1];
            r = ((uchar *)(tmpplate->imageData + row * tmpplate->widthStep))[col * tmpplate->nChannels + 2];
            if(choose==1)
                tmptar=b+g+r;
            else
                tmptar=b-g-r;
            if(tmptar>tar){
                tar=tmptar;
                pointlist[3].x=row;
                pointlist[3].y=col;
            }
        }
    }
}

int isbule(int col,int row,CvPoint bule[],CvPoint white[],IplImage *src){
    int b_dis,w_dis,testb,testr,testg,n = 0;
    if(col<=src->width/2&&row<=src->height/2) n=0;
    if(col>=src->width/2&&row<=src->height/2) n=1;
    if(col<=src->width/2&&row>=src->height/2) n=2;
    if(col>=src->width/2&&row>=src->height/2) n=3;

    testb = ((uchar *)(src->imageData + row * src->widthStep))[col * src->nChannels + 0];
    testg = ((uchar *)(src->imageData + row * src->widthStep))[col * src->nChannels + 1];
    testr = ((uchar *)(src->imageData + row * src->widthStep))[col * src->nChannels + 2];

    int bb = ((uchar *)(src->imageData + bule[n].x * src->widthStep))[bule[n].y * src->nChannels + 0];
    int bg = ((uchar *)(src->imageData +bule[n].x * src->widthStep))[bule[n].y * src->nChannels + 1];
    int br = ((uchar *)(src->imageData +bule[n].x * src->widthStep))[bule[n].y * src->nChannels + 2];
    int wb = ((uchar *)(src->imageData + white[n].x * src->widthStep))[white[n].y * src->nChannels + 0];
    int wg = ((uchar *)(src->imageData +white[n].x * src->widthStep))[white[n].y * src->nChannels + 1];
    int wr = ((uchar *)(src->imageData +white[n].x * src->widthStep))[white[n].y * src->nChannels + 2];

    b_dis=sqrt((testb-bb)*(testb-bb)+(testr-br)*(testr-br)+(testg-bg)*(testg-bg));
    w_dis=sqrt((testb-wb)*(testb-wb)+(testr-wr)*(testr-wr)+(testg-wg)*(testg-wg));
    if(b_dis<(w_dis*0.7))
        return 1;
    else
        return 0;
}

void prechar(IplImage * charplate ,CvRect charrect[],IplImage * charimg[]){
    for(int i=0;i<7;i++){
        IplImage *charroi,*charresize,*chargary,*charfine;
        CvRect fineroi;
        charroi=cvCreateImage(cvSize(charrect[i].width,charrect[i].height),charplate->depth,charplate->nChannels);
        cvSetImageROI(charplate,charrect[i]);
        cvCopy(charplate,charroi,NULL);
        cvResetImageROI(charroi);
        charresize=cvCreateImage(cvSize(20,40),charroi->depth,charroi->nChannels);
        cvResize(charroi,charresize,CV_INTER_LINEAR);
        chargary = cvCreateImage(cvGetSize(charresize), IPL_DEPTH_8U, 1);
        cvCvtColor(charresize, chargary,CV_BGR2GRAY);
        cvSmooth(chargary,chargary,CV_MEDIAN);
        cvThreshold(chargary,chargary, otsu(chargary),255, CV_THRESH_BINARY);

        fineroi=refinechar(chargary);
        charfine=cvCreateImage(cvSize(fineroi.width,fineroi.height),chargary->depth,chargary->nChannels);
        cvSetImageROI(chargary,fineroi);
        cvCopy(chargary,charfine,NULL);
        cvResetImageROI(chargary);
        cvResize(charfine,charimg[i],CV_INTER_LINEAR);
        cvThreshold(charimg[i],charimg[i], otsu(charimg[i]),255, CV_THRESH_BINARY);
        cvReleaseImage(&charroi);
        cvReleaseImage(&charresize);
        cvReleaseImage(&chargary);
        cvReleaseImage(&charfine);
    }
}

CvRect refinechar(IplImage *src){
    int xmax=src->width,ymax=src->height,xmin=0,ymin=0;
    int count=0,t;
    for(int col=0;col<src->width;col++){
        count=0;
        for(int row=0;row<src->height;row++){
            t=((uchar *)(src->imageData + row * src->widthStep))[col];
            if(t==255)
                count++;
        }
        if(count>4){
            xmin=col;
            break;
        }
    }
    for(int col=src->width;col>=0;col--){
       count=0;
        for(int row=0;row<src->height;row++){
            t=((uchar *)(src->imageData + row * src->widthStep))[col];
            if(t==255)
                count++;
        }
        if(count>4){
            xmax=col;
            break;
        }
    }
    for(int row=int(src->height/2);row>=0;row--){
        count=0;
        for(int col=0;col<src->width;col++){
            t=((uchar *)(src->imageData + row * src->widthStep))[col];
            if(t==255)
                count++;
        }
        if(count<5){
            ymin=row;
            break;
        }
    }
    for(int row=int(src->height/2);row<src->height;row++){
        count=0;
        for(int col=0;col<src->width;col++){
            t=((uchar *)(src->imageData + row * src->widthStep))[col];
            if(t==255)
                count++;
        }
        if(count<5){
            ymax=row;
            break;
        }
    }
    CvRect fine;
    fine.x=xmin;fine.y=ymin;fine.width=xmax-xmin;fine.height=ymax-ymin;
    if(fine.width==0){
        fine.x=0;
        fine.width=src->width;
    }
    if(fine.height==0){
        fine.y=0;
        fine.height=src->height;
    }
     return fine;
}

int crossckeckchar(vector<Chargrade> gradelist,IplImage *charpic){
    int verify=0,select; //"1":统计每一行白色到白色的宽度
    // verify 1
    //黑到白的跳变次数+白色宽度的比例
    if(verify==0){
        int count1=0,count2=0,count3=0;
        for(int row=0;row<charpic->height;row++){
            count1=0;
            int begin=0,end=charpic->width;
            for(int col=0;col<(charpic->width-1);col++){
                int fore,back;
                fore=CV_IMAGE_ELEM(charpic, uchar, row, col);
                back=CV_IMAGE_ELEM(charpic, uchar, row, col+1);
                if(CV_IMAGE_ELEM(charpic, uchar, row, col)==0&&CV_IMAGE_ELEM(charpic, uchar, row, col+1)==255){
                    if(begin==0)
                        begin=col;
                    count1++;
                }

                if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255&&CV_IMAGE_ELEM(charpic, uchar, row, col+1)==0)
                    end=col;
            }

            if(CV_IMAGE_ELEM(charpic, uchar, row, 0)==255)
                count1++;
            if(count1<=1){
                int dis;
                dis=end-begin;
                if(dis<0)
                    dis=end;
                if(dis>14)
                    count3++;
             }
             else
                 count2++;
         }
         if(count2<2&&count3>=25)
             return 1;
    }
    select=0;
    /*while(verify==0){
    int error=0,row,col,count,count2;
    switch (gradelist[select].charnum)
    {
        case 1:
           select++;
           verify=0;
           break;
        case 4://“4”：统计左上角三角形内白色像素的数目
            count=0;
            for(int row=0;row<20;row++){
                for(col=0;col<(10-row/2);col++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
            }
            if(count>5){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
         case 6:
            count=0,count2=0,error=0;
            for(int row=3;row<18;row++){//统计上下两半右边部分黑到白的跳变的次数差
                for(col=10;col<19;col++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==0&&CV_IMAGE_ELEM(charpic, uchar, row, col+1)==255)
                        count++;
                }
            }
            for(int row=23;row<38;row++){
                for(col=10;col<18;col++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==0&&CV_IMAGE_ELEM(charpic, uchar, row, col+1)==255)
                        count2++;
                }
            }
            if((count2-count)<=5)
                error=1;

            count=0,count2=0;//统计下面部分左右两边白色像素的差值
            for(int row=23;row<38;row++){
                for(col=0;col<10;col++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
            }
            for(int row=23;row<38;row++){
                for(col=10;col<20;col++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count2++;
                }
            }
            if(abs(count-count2)>10)
                error=1;
            if(error!=0){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
        case 28://U width/2上面如果出现了白到黑的跳变则不是U
            for(row=0;row<(charpic->height-1);row++){
                if(CV_IMAGE_ELEM(charpic, uchar, row, int(charpic->width/2))==255&&CV_IMAGE_ELEM(charpic, uchar, row+1, int(charpic->width/2))==0){
                    break;
                }
            }
            if(row<int(charpic->height/2)){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
        case 29://U width/2上面如果出现了白到黑的跳变则不是U
            for(row=0;row<(charpic->height-1);row++){
                if(CV_IMAGE_ELEM(charpic, uchar, row, int(charpic->width/2))==255&&CV_IMAGE_ELEM(charpic, uchar, row+1, int(charpic->width/2))==0){
                    break;
                }
            }
            if(row<int(charpic->height/2)){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
        case 11:
            count=0;count2=0;
            for(col=0;col<5;col++){
                for(int row=0;row<5;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
            }
            for(col=0;col<5;col++){
                for(int row=35;row<40;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
            }
            for(col=15;col<20;col++){
                for(int row=0;row<5;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count2++;
                }
            }
            for(col=15;col<20;col++){
                for(int row=35;row<40;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count2++;
                }
            }
            if((count-count2)<10){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
        case 16:
            count=0;count2=0;
            for(col=0;col<10;col++){
                for(int row=10;row<20;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
            }

            for(col=10;col<20;col++){
                for(int row=10;row<20;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count2++;
                }
            }
            if((count>count2)<10){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
        case 13://如果为D，其左边一定全为白
            for(col=0;col<charpic->width;col++){
                int count=0;
                for(int row=0;row<charpic->height;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
                if(count==charpic->height)
                    break;
            }
            if(col>1){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
        case 21://如果为M的化左右两边没有黑到白的跳变点
            count=0,count2=0;
            for(col=0;col<20;col++){
                for(int row=0;row<charpic->height;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count++;
                }
            }
            for(col=20;col<40;col++){
                for(int row=0;row<charpic->height;row++){
                    if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                        count2++;
                }
            }
            if((count-count2)>10){
                select++;
                verify=0;
            }
            else
                verify=1;
            break;
         //case 21://如果为M的化左右两边没有黑到白的跳变点
             for(int row=2;row<charpic->height-3;row++){
                 if(CV_IMAGE_ELEM(charpic, uchar, row, 1)==0&&CV_IMAGE_ELEM(charpic, uchar, row+1, 1)==255){
                     error=1;
                     break;
                 }
                 if(CV_IMAGE_ELEM(charpic, uchar, row, charpic->width-2)==0&&CV_IMAGE_ELEM(charpic, uchar, row+1, charpic->width-2)==255){
                     error=1;
                     break;
                 }
             }
             if(error==1){
                 select++;
                 verify=0;
             }
             else
                 verify=1;
             // break;
            case 27:
                count=0,count2=0;
                for(col=0;col<10;col++){
                    for(int row=0;row<40;row++){
                        if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                            count++;
                    }
                }
                for(col=10;col<20;col++){
                    for(int row=0;row<40;row++){
                        if(CV_IMAGE_ELEM(charpic, uchar, row, col)==255)
                            count2++;
                    }
                }
                if(abs((count-count2))>5){
                    select++;
                    verify=0;
                }
                else
                    verify=1;
                break;
            default:
                verify=1;
                break;
        }
    }*/

    return gradelist[select].charnum;
}

#endif
