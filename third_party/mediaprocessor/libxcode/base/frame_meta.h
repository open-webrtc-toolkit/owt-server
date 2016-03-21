/* <COPYRIGHT_TAG> */
#ifdef ENABLE_VA

#ifndef FRAME_META_H
#define FRAME_META_H
#include <list>
//FrameMeta Stream Layout//

/*=====int VideoId==========
  =====int FrameId==========
  =====int PayloadLen=======
  =====int PayloadIndex=====
  =====int Feature Len=========
  =====int FeatureIndex=========
  =====ROI Data=============
  =====ES Data==============
*/

struct ROI {
    int tl_x;
    int tl_y;
    int br_x;
    int br_y;
};

typedef std::list<struct ROI*> LIST_ROI;

struct FrameMeta {
    // video frame vector.
    int VideoId;
    int FrameId;
    int FeatureType;
    int NumOfROI;
    LIST_ROI ROIList;
    int DescriptorLen;
    unsigned char* Descriptor;
    // video frame bit stream.
    int BitStreamLen;
    unsigned char* FrameBitStream;
};

void FrameMetaRelease(FrameMeta* frame);
int VectorStreamSize(FrameMeta* frame);
int Vector2Stream(FrameMeta* frame, char* buffer, int len);
int Stream2Vector(char* buffer, int len, FrameMeta* frame);
#endif

#endif
