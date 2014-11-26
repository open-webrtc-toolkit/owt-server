/* <COPYRIGHT_TAG> */

/**
 *\file app_util.h
 *\brief Definition for utility */

#ifndef _APP_UTIL_H_
#define _APP_UTIL_H_

#include <string>
#include "app_def.h"

class Utility
{
private:
    Utility() {};
    ~Utility() {};

    static const std::string mod_name_[APP_END_MOD];
    static const std::string level_name_[SYS_DEBUG + 1];

public:
    static const std::string &mod_name(AppModId);
    static AppModId mod_id(const char *);
    static const std::string &level_name(TraceLevel);
};

#endif
