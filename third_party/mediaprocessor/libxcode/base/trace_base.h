/* <COPYRIGHT_TAG> */

/**
 *\file trace_base.h
 *\brief Implementation for Trace base class
 */

#ifndef _TRACE_BASE_H_
#define _TRACE_BASE_H_

/**
 * This class will use for control each object trace, like decoder or encoeer;
 * use derived class from it, and pass this object to trace, trace module
 * can filter the trace by this object
 */

class TraceBase
{
private:
    bool enable_;

protected:
    TraceBase(): enable_(false) {}

public:
    const bool enable() {
        return enable_;
    } const
    void enable(const bool flag) {
        enable_ = flag;
    }
};

#endif
