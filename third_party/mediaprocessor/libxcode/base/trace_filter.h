/* <COPYRIGHT_TAG> */

/**
 *\file trace_filter.h
 *\brief Definition for ModFilter & TraceFilter
 */

#ifndef _TRACE_FILTER_H_
#define _TRACE_FILTER_H_

#include "app_def.h"

class TraceInfo;
class TraceBase;

class ModFilter
{
public:
    ModFilter(): enable_(false), level_(SYS_INFORMATION) {}

    void enable(bool flag) {
        enable_ = flag;
    }
    const bool enable() {
        return enable_;
    }

    const bool enable(TraceLevel level) {
        if (enable_ && level <= level_) {
            return true;
        } else {
            return false;
        }
    }

    void level(TraceLevel level) {
        level_ = level;
    }
    const TraceLevel level() {
        return level_;
    }

private:
    bool enable_;
    TraceLevel level_;
};

class TraceFilter
{
private:
    bool enable_;
    TraceLevel level_;
    ModFilter filters_[APP_END_MOD];

public:
    TraceFilter(): enable_(false), level_(SYS_ERROR) {}

    TraceLevel level(AppModId mod) {
        return filters_[mod].level();
    }
    TraceLevel level() {
        return level_;
    }
    void level(TraceLevel level) {
        level_ = level;
    }

    void enable(bool flag) {
        enable_ = flag;
    }
    bool enable() {
        return enable_;
    }

    void enable(AppModId, TraceLevel);
    void enable_all(TraceLevel);
    bool enable(AppModId mod) {
        return filters_[mod].enable();
    }
    void disable(AppModId mod) {
        filters_[mod].enable(false);
    }

    bool enable(TraceInfo *);
    bool enable(TraceInfo *, TraceBase *);
    bool enable(AppModId, TraceLevel, int);
    bool enable(AppModId, TraceLevel, TraceBase *);
};

#endif

