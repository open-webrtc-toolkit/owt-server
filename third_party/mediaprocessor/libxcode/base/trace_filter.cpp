/* <COPYRIGHT_TAG> */

/**
 *\file trace_filter.cpp
 *\brief Implementation for TraceFilter class
 */

#include "trace_filter.h"
#include "trace_base.h"
#include "trace_info.h"

void TraceFilter::enable(AppModId mod, TraceLevel level)
{
    filters_[mod].level(level);
    filters_[mod].enable(true);
}

void TraceFilter::enable_all(TraceLevel level)
{
    for (int idx = 0; idx < APP_END_MOD; idx++) {
        filters_[idx].level(level);
        filters_[idx].enable(true);
    }
}

bool TraceFilter::enable(TraceInfo *trc)
{
    if (trc->level() <= level_) {
        return true;
    }

    if (enable()) {
        return true;
    } else {
        return filters_[trc->mod()].enable(trc->level());
    }
}

bool TraceFilter::enable(TraceInfo *trc, TraceBase *obj)
{
    if (enable(trc)) {
        return true;
    } else {
        return obj->enable();
    }
}

bool TraceFilter::enable( AppModId mod, TraceLevel level, int flag)
{
    if (level <= level_) {
        return true;
    }

    if (enable()) {
        return true;
    } else {
        return filters_[mod].enable(level);
    }
}

bool TraceFilter::enable(AppModId mod, TraceLevel level, TraceBase *obj)
{
    if (enable(mod, level, 0)) {
        return true;
    } else {
        return obj->enable();
    }
}

