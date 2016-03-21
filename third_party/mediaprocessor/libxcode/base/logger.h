/* <COPYRIGHT_TAG> */

#ifndef _MLOG_H_
#define _MLOG_H_

#include <stdio.h>
#include <stdlib.h>

#include "debug_info.h"

#define NODE "\033[m"
#define RED "\033[0;32;31m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[0;32;32m"
#define BLUE "\033[0;32;34m"
#define BLACK "\033[0;32;30"


#define DECLARE_MLOGINSTANCE() \
    static LogInstance* logInstance

#define DEFINE_MLOGINSTANCE(moduleName) \
    static LogInstance* logInstance = LogFactory::GetLogFactory()->CreateLogInstance(moduleName)
    
#define DEFINE_MLOGINSTANCE_CLASS(namespace, moduleName) \
    LogInstance*  namespace::logInstance = LogFactory::GetLogFactory()->CreateLogInstance(moduleName)


#define MLOG_FATAL(format, args...) \
    if (logInstance->isFatalEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::FATAL, RED, format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::FATAL, RED, format, ##args); \
    }

#define MLOG_ERROR(format, args...) \
    if (logInstance->isErrorEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::ERROR, RED,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::ERROR, RED, format, ##args); \
    }

#define MLOG_WARNING(format, args...) \
    if (logInstance->isWarningEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::WARNING, YELLOW,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::WARNING, YELLOW, format, ##args); \
    }

#define MLOG_INFO(format, args...) \
    if (logInstance->isInfoEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::INFO, GREEN,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::INFO, GREEN, format, ##args); \
    }

#define MLOG_TRACE(format, args...) \
    if (logInstance->isTraceEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::TRACE, "",  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::TRACE, "", format, ##args); \
    }

#define MLOG_DEBUG(format, args...) \
    if (logInstance->isDebugEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::DEBUG,BLUE ,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::DEBUG, BLUE, format, ##args); \
    }

#define MLOG(format, args...) \
    logInstance->LogPrintf(format, ##args);

#endif //_MLOG_H_
