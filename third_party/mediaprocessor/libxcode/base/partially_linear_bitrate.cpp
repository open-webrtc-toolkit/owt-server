/* <COPYRIGHT_TAG> */

#include <memory.h>
#include <stdio.h>

#include "partially_linear_bitrate.h"

PartiallyLinearBitrate::PartiallyLinearBitrate()
: m_pX()
, m_pY()
, m_nPoints()
, m_nAllocated()
{

}

PartiallyLinearBitrate::~PartiallyLinearBitrate()
{
    delete []m_pX;
    m_pX = NULL;
    delete []m_pY;
    m_pY = NULL;
}

void PartiallyLinearBitrate::AddPair(double x, double y)
{
    //duplicates searching
    for (unsigned int i = 0; i < m_nPoints; i++)
    {
        if (m_pX[i] == x)
            return;
    }
    if (m_nPoints == m_nAllocated)
    {
        m_nAllocated += 20;
        double * pnew;
        pnew = new double[m_nAllocated];
        memcpy(pnew, m_pX, sizeof(double) * m_nPoints);
        delete [] m_pX;
        m_pX = pnew;

        pnew = new double[m_nAllocated];
        memcpy(pnew, m_pY, sizeof(double) * m_nPoints);
        delete [] m_pY;
        m_pY = pnew;
    }
    m_pX[m_nPoints] = x;
    m_pY[m_nPoints] = y;

    m_nPoints ++;
}

double PartiallyLinearBitrate::at(double x)
{
    if (m_nPoints < 2)
    {
        return 0;
    }
    bool bwasmin = false;
    bool bwasmax = false;

    unsigned int maxx = 0;
    unsigned int minx = 0;
    unsigned int i;

    for (i=0; i < m_nPoints; i++)
    {
        if (m_pX[i] <= x && (!bwasmin || m_pX[i] > m_pX[maxx]))
        {
            maxx = i;
            bwasmin = true;
        }
        if (m_pX[i] > x && (!bwasmax || m_pX[i] < m_pX[minx]))
        {
            minx = i;
            bwasmax = true;
        }
    }

    //point on the left
    if (!bwasmin)
    {
        for (i=0; i < m_nPoints; i++)
        {
            if (m_pX[i] > m_pX[minx] && (!bwasmin || m_pX[i] < m_pX[minx]))
            {
                maxx = i;
                bwasmin = true;
            }
        }
    }

    //point on the right
    if (!bwasmax)
    {
        for (i=0; i < m_nPoints; i++)
        {
            if (m_pX[i] < m_pX[maxx] && (!bwasmax || m_pX[i] > m_pX[minx]))
            {
                minx = i;
                bwasmax = true;
            }
        }
    }

    //linear interpolation
    return (x - m_pX[minx])*(m_pY[maxx] - m_pY[minx]) / (m_pX[maxx] - m_pX[minx]) + m_pY[minx];
}

