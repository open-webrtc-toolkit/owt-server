/* <COPYRIGHT_TAG> */

/**
 *\file trace_info.h
 *\brief Implementation for trace info class
 */

#ifndef _TRACE_INFO_H_
#define _TRACE_INFO_H_

#include <string>
#include <ctime>
#include "app_def.h"

class TraceInfo
{
private:
    AppModId mod_;
    TraceLevel level_;
    time_t time_;
    std::string desc_;

public:
    TraceInfo(AppModId mod, TraceLevel level, std::string desc):
        mod_(mod), level_(level) {
        desc_.erase();
        desc_ = desc;
        time_ = std::time(0);
    }

    AppModId mod() {
        return mod_;
    }
    TraceLevel level() {
        return level_;
    }
    time_t *time() {
        return &time_;
    }
    std::string &desc() {
        return desc_;
    }
};

#endif

