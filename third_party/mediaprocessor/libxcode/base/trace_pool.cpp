/* <COPYRIGHT_TAG> */

/**
 *\file trace_pool.cpp
 *\brief Implementation for trace pool
 */

#include "trace_pool.h"
#include <sstream>
#include <iostream>
#include "app_util.h"
#include "trace_info.h"

using namespace std;

void TracePool::add(TraceInfo *trc)
{
    if (NULL == trc) {
        TRC_DEBUG("Input parameter is NULL\n");
        return;
    }

    if (vect_.size() < MAX_SIZE) {
        vect_.push_back(trc);
        return;
    }

    delete vect_[0];
    vect_[0] = NULL;
    vect_.erase(vect_.begin(), vect_.begin() + 1);
    vect_.push_back(trc);
}

TraceInfo *TracePool::get(unsigned int index) const
{
    if (index < vect_.size()) {
        return vect_[index];
    }

    return NULL;
}

void TracePool::clear()
{
    for (unsigned int idx = 0; idx < vect_.size(); idx++) {
        delete vect_[idx];
        vect_[idx] = NULL;
    }

    vect_.clear();
}

void TracePool::dump()
{
    ostringstream buff;
    buff << "Mod\tLevel\tTime\tDesc\n";
    char timestr[40] = {0};
    tm *timeinfo  = NULL;

    for (unsigned int i = 0; i < vect_.size(); i++) {
        timeinfo = localtime(vect_[i]->time());
        if (timeinfo) {
            strftime (timestr, 40, "%X", timeinfo);
        }
        buff << "<" << Utility::mod_name(vect_[i]->mod()) << "><"
             << Utility::level_name(vect_[i]->level()) << "><"
             << timestr << ">:" << vect_[i]->desc() << "\n";
    }

    cout << buff.str();
}

