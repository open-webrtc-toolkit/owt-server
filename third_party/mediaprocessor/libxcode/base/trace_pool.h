/* <COPYRIGHT_TAG> */

/**
 *\file trace_pool.h
 *\brief Definition for Trace pool
 */


#ifndef _TRACE_MEM_H_
#define _TRACE_MEM_H_

#include <vector>

class TraceInfo;

class TracePool
{
public:
    void add(TraceInfo *);
    void clear();
    TraceInfo *get(unsigned int index) const;
    void dump();

private:
    static const unsigned int MAX_SIZE = 1000;
    typedef std::vector<TraceInfo *> TracePoolVec;
    TracePoolVec vect_;

};

#endif

