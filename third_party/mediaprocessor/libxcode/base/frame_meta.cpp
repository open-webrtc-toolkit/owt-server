/* <COPYRIGHT_TAG> */
#include "frame_meta.h"

#ifdef ENABLE_VA

#include "string.h"
#include "stdio.h"

int VectorStreamSize(FrameMeta* frame)
{
    int size_of_int = sizeof(int);
    int size_of_roi = sizeof(ROI);
    int len;
    if (frame->FeatureType == 1) {
        len = 6 * size_of_int + frame->DescriptorLen;
    } else {
        len = 6 * size_of_int + frame->NumOfROI * size_of_roi;
    }

    return len;
}

void FrameMetaRelease(FrameMeta* frame)
{
    if (frame->FeatureType == 1) {
        if (frame->Descriptor) {
            delete frame->Descriptor;
            frame->Descriptor = NULL;
        }
    } else {
        LIST_ROI::iterator i;
        for (i = frame->ROIList.begin(); i != frame->ROIList.end(); i++) {
            delete *i;
        }
        frame->ROIList.clear();
    }

    return;
}

int Vector2Stream(FrameMeta* frame, char* buffer, int len)
{
    int size_of_int = sizeof(int);
    int size_of_roi = sizeof(ROI);
    int idx = 0;

    if ((frame == NULL) || (buffer == NULL) || (len < 0)) {
        return -1;
    }
    // Write video id
    if (idx + size_of_int > len) {
        return -1;
    }
    memcpy(buffer+idx, &(frame->VideoId), size_of_int);
    idx = idx + size_of_int;

    // Write frame id
    if (idx + size_of_int > len) {
        return -1;
    }
    memcpy(buffer+idx, &(frame->FrameId), size_of_int);
    idx = idx + size_of_int;

    // Write bit stream length
    if (idx + size_of_int > len) {
        return -1;
    }
    memcpy(buffer+idx, &(frame->BitStreamLen), size_of_int);
    idx = idx + size_of_int;
    // Write bit stream Index
    if (idx + size_of_int > len) {
        return -1;
    }

    int head;
    if (frame->FeatureType == 1) {
        head = 6 * size_of_int + frame->DescriptorLen;
    } else {
        head = 6 * size_of_int + frame->NumOfROI * size_of_roi;
    }
    memcpy(buffer+idx, &head, size_of_int);
    idx = idx + size_of_int;

    // Write number of roi.
    if (idx + size_of_int > len) {
        return -1;
    }
    if (frame->FeatureType == 1) {
        memcpy(buffer+idx, &(frame->DescriptorLen), size_of_int);
    } else {
        memcpy(buffer+idx, &(frame->NumOfROI), size_of_int);
    }
    idx = idx + size_of_int;

    // Write the ROI index or feature index
    int offset = idx + size_of_int;
    memcpy(buffer+idx, &offset, size_of_int);
    idx = idx + size_of_int;

    if (frame->FeatureType == 1) {
        // write descriptor
        if (idx + frame->DescriptorLen > len) {
            return -1;
        }
        if (frame->Descriptor && frame->DescriptorLen > 0) {
            memcpy(buffer+idx, frame->Descriptor, frame->DescriptorLen);
            idx = idx + frame->DescriptorLen;
        }
    } else {
        // Write roi
        LIST_ROI::iterator i;
        for(i = frame->ROIList.begin(); i != frame->ROIList.end(); i++) {
            if (idx + size_of_roi > len) {
                return -1;
            }
            memcpy(buffer+idx, *i, size_of_roi);
            idx = idx + size_of_roi;
        }
    }
    return 0;
}
int Stream2Vector(char* buffer, int len, FrameMeta* frame)
{
    int size_of_int = sizeof(int);
    int size_of_roi = sizeof(ROI);
    int idx = 0;

    if((frame == NULL)||(buffer == NULL) || (len < 0)){
        return -1;
    }

    // fill video id
    memcpy(&(frame->VideoId), buffer+idx, size_of_int);
    idx = idx + size_of_int;
    if(idx > len) {
        return -1;
    }

    // fill frame id
    memcpy(&(frame->FrameId), buffer+idx, size_of_int);
    idx = idx + size_of_int;
    if(idx > len) {
        return -1;
    }

    // fill the payload len
    memcpy(&(frame->BitStreamLen), buffer+idx, size_of_int);
    idx = idx + size_of_int;
    if (idx > len) {
        return -1;
    }

    // fill the paylaod pointer.
    int offset = 0;
    memcpy(&(offset), buffer+idx, size_of_int);
    frame->FrameBitStream = (unsigned char*)(buffer + offset);
    idx = idx + size_of_int;
    if (idx > len) {
        return -1;
    }

    // fill num of roi
    memcpy(&(frame->NumOfROI), buffer+idx, size_of_int);
    idx = idx + size_of_int;
    if(idx>len) {
        return -1;
    }

    // the ROI index
    int roi_offset = 0;
    memcpy(&roi_offset, buffer+idx, size_of_int);
    idx = idx + size_of_int;
    if (idx > len) {
        return -1;
    }

    idx = roi_offset;
    // fill the roi list
    for(int i = 0; i < frame->NumOfROI; i++){
        struct ROI* roi = new struct ROI;
        if (roi == NULL) {
            FrameMetaRelease(frame);
            frame->ROIList.clear();
            return -1;
        }
        memcpy(roi, buffer+idx, size_of_roi);
        idx = idx + size_of_roi;
        if (idx>len) {
            FrameMetaRelease(frame);
            delete roi;
            return -1;
        }
        frame->ROIList.push_back(roi);
    }
    return 0;
}

#endif
