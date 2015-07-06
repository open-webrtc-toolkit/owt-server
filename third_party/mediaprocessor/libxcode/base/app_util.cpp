/* <COPYRIGHT_TAG> */

/**
 *\file app_util.cpp
 *\brief Implementation of utility */

#include <string.h>
#include "app_util.h"
using namespace std;

const string Utility::mod_name_[APP_END_MOD] = {
    "",
    "STRM",  /* 0x01 */
    "F2F",  /* 0x02 */
    "FRMW",  /* 0x03 */
    "H264D",  /* 0x04 */
    "H264E",  /* 0x05 */
    "MPEG2D",  /* 0x06 */
    "MPEG2E",  /* 0x07 */
    "VC1D",  /* 0x08 */
    "VC1E",  /* 0x09 */
    "VP8D",  /* 0x0a */
    "V8E",  /* 0x0b */
    "VPP",  /* 0x0c */
    "MSMT",  /* 0x0d */
    "TRC"  /* 0x0e */
};

const string Utility::level_name_[SYS_DEBUG + 1] = {
    "",
    "ERROR",
    "WARNI",
    "INFO",
    "DEBUG"
};

const string &Utility::mod_name(AppModId id)
{
    if (id < APP_END_MOD) {
        return mod_name_[id];
    }

    return mod_name_[0];
}

AppModId Utility::mod_id(const char *modName)
{
    for (int idx = 0; idx < APP_END_MOD; idx++) {
        if (0 == strcmp(mod_name_[idx].c_str(), modName)) {
            return (AppModId)idx;
        }
    }

    return APP_END_MOD;
}

const string &Utility::level_name(TraceLevel level)
{
    if (level >= SYS_ERROR && level <= SYS_DEBUG) {
        return level_name_[level];
    }

    return level_name_[0];
}


