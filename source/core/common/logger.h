/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode

#ifndef ERIZO_SRC_ERIZO_LOGGER_H_
#define ERIZO_SRC_ERIZO_LOGGER_H_

#include <stdlib.h>
#include <stdio.h>

#include <log4cxx/logger.h>
#include <log4cxx/helpers/exception.h>

 #define DECLARE_LOGGER() \
 static log4cxx::LoggerPtr logger;

 #define DEFINE_LOGGER(namespace, logName) \
 log4cxx::LoggerPtr namespace::logger = log4cxx::Logger::getLogger( logName );

 #define DEFINE_TEMPLATE_LOGGER(templateArg, namespace, logName) \
 templateArg log4cxx::LoggerPtr namespace::logger = log4cxx::Logger::getLogger( logName );

#define ELOG_MAX_BUFFER_SIZE 30000

#define SPRINTF_ELOG_MSG(buffer, fmt, args...) \
    char buffer[ELOG_MAX_BUFFER_SIZE]; \
    snprintf( buffer, ELOG_MAX_BUFFER_SIZE, fmt, ##args );

// older versions of log4cxx don't support tracing
#ifdef LOG4CXX_TRACE
#define ELOG_TRACE2(logger, fmt, args...) \
    if (logger->isTraceEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_TRACE( logger, __tmp ); \
    }
#else
#define ELOG_TRACE2(logger, fmt, args...) \
    if (logger->isDebugEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_DEBUG( logger, __tmp ); \
    }
#endif

#define ELOG_DEBUG2(logger, fmt, args...) \
    if (logger->isDebugEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_DEBUG( logger, __tmp ); \
    }

#define ELOG_INFO2(logger, fmt, args...) \
    if (logger->isInfoEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_INFO( logger, __tmp ); \
    }

#define ELOG_WARN2(logger, fmt, args...) \
    if (logger->isWarnEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_WARN( logger, __tmp ); \
    }

#define ELOG_ERROR2(logger, fmt, args...) \
    if (logger->isErrorEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_ERROR( logger, __tmp ); \
    }

#define ELOG_FATAL2(logger, fmt, args...) \
    if (logger->isFatalEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, ##args ); \
        LOG4CXX_FATAL( logger, __tmp ); \
    }


#define ELOG_TRACE(fmt, args...) \
    ELOG_TRACE2( logger, fmt, ##args );

#define ELOG_DEBUG(fmt, args...) \
    ELOG_DEBUG2( logger, fmt, ##args );

#define ELOG_INFO(fmt, args...) \
    ELOG_INFO2( logger, fmt, ##args );

#define ELOG_WARN(fmt, args...) \
    ELOG_WARN2( logger, fmt, ##args );

#define ELOG_ERROR(fmt, args...) \
    ELOG_ERROR2( logger, fmt, ##args );

#define ELOG_FATAL(fmt, args...) \
    ELOG_FATAL2( logger, fmt, ##args );

//this
#define ELOG_TRACE_T(fmt, args...) \
    ELOG_TRACE2( logger, "(%p)" fmt, this, ##args );

#define ELOG_DEBUG_T(fmt, args...) \
    ELOG_DEBUG2( logger, "(%p)" fmt, this, ##args );

#define ELOG_INFO_T(fmt, args...) \
    ELOG_INFO2( logger, "(%p)" fmt, this, ##args );

#define ELOG_WARN_T(fmt, args...) \
    ELOG_WARN2( logger, "(%p)" fmt, this, ##args );

#define ELOG_ERROR_T(fmt, args...) \
    ELOG_ERROR2( logger, "(%p)" fmt, this, ##args );

#define ELOG_FATAL_T(fmt, args...) \
    ELOG_FATAL2( logger, "(%p)" fmt, this, ##args );

#ifdef LOG4CXX_TRACE
#define ELOG_IS_TRACE_ENABLED() \
    logger->isTraceEnabled()
#else
#define ELOG_IS_TRACE_ENABLED() \
    logger->isDebugEnabled()
#endif

#define ELOG_IS_DEBUG_ENABLED() \
    logger->isDebugEnabled()

#define ELOG_IS_INFO_ENABLED() \
    logger->isInfoEnabled()

#define ELOG_IS_WARN_ENABLED() \
    logger->isWarnEnabled()

#define ELOG_IS_ERROR_ENABLED() \
    logger->isErrorEnabled()

#define ELOG_IS_FATAL_ENABLED() \
    logger->isFatalEnabled()

#endif  /* __ELOG_H__ */
