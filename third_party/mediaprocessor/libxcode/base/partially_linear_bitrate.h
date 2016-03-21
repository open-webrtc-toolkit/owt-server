/* <COPYRIGHT_TAG> */

#ifndef _PARTIALLY_LINEAR_BITRATE_H_
#define _PARTIALLY_LINEAR_BITRATE_H_

//piecewise linear function for bitrate approximation
class PartiallyLinearBitrate {
    double *m_pX;
    double *m_pY;
    unsigned int  m_nPoints;
    unsigned int  m_nAllocated;

public:
    PartiallyLinearBitrate();
    ~PartiallyLinearBitrate();

    void AddPair(double x, double y);
    double at(double);

private:
    PartiallyLinearBitrate(const PartiallyLinearBitrate&);
    void operator=(const PartiallyLinearBitrate&);
};

#endif

