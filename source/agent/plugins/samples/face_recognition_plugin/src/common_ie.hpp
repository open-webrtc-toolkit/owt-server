/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

/**
 * @brief a header file with common samples functionality
 * @file common.hpp
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <limits>
#include <random>
#include <cctype>
#include <functional>
#include <time.h>
#include <iostream>
#include <iomanip>

#include <algorithm>
#include <chrono>

#include <inference_engine.hpp>

#ifndef UNUSED
  #ifdef WIN32
    #define UNUSED
  #else
    #define UNUSED  __attribute__((unused))
  #endif
#endif

/**
 *  * @brief Gets filename without extension
 *   * @param filepath - full file name
 *    * @return filename without extension
 *     */
static std::string fileNameNoExt(const std::string &filepath) {
        auto pos = filepath.rfind('.');
            if (pos == std::string::npos) return filepath;
                return filepath.substr(0, pos);
}

/**
 * * @brief Get extension from filename
 * * @param filename - name of the file which extension should be extracted
 * * @return string with extracted file extension
 * */
inline std::string fileExt(const std::string& filename) {
        auto pos = filename.rfind('.');
            if (pos == std::string::npos) return "";
                return filename.substr(pos + 1);
}

static UNUSED std::ostream &operator<<(std::ostream &os, const InferenceEngine::Version *version) {
    os << "\n\tAPI version ............ ";
    if (nullptr == version) {
        os << "UNKNOWN";
    } else {
        os << version->apiVersion.major << "." << version->apiVersion.minor;
        if (nullptr != version->buildNumber) {
            os << "\n\t" << "Build .................. " << version->buildNumber;
        }
        if (nullptr != version->description) {
            os << "\n\t" << "Description ....... " << version->description;
        }
    }
    return os;
}

/**
 * @class PluginVersion
 * @brief A PluginVersion class stores plugin version and initialization status
 */
struct PluginVersion : public InferenceEngine::Version {
    bool initialized = false;

    explicit PluginVersion(const InferenceEngine::Version *ver) {
        if (nullptr == ver) {
            return;
        }
        InferenceEngine::Version::operator=(*ver);
        initialized = true;
    }

    operator bool() const noexcept {
        return initialized;
    }
};

static UNUSED std::ostream &operator<<(std::ostream &os, const PluginVersion &version) {
    os << "\tPlugin version ......... ";
    if (!version) {
        os << "UNKNOWN";
    } else {
        os << version.apiVersion.major << "." << version.apiVersion.minor;
    }

    os << "\n\tPlugin name ............ ";
    if (!version || version.description == nullptr) {
        os << "UNKNOWN";
    } else {
        os << version.description;
    }

    os << "\n\tPlugin build ........... ";
    if (!version || version.buildNumber == nullptr) {
        os << "UNKNOWN";
    } else {
        os << version.buildNumber;
    }

    return os;
}

inline void printPluginVersion(InferenceEngine::InferenceEnginePluginPtr ptr, std::ostream& stream) {
    const PluginVersion *pluginVersion;
    ptr->GetVersion((const InferenceEngine::Version*&)pluginVersion);
    stream << pluginVersion << std::endl;
}

static UNUSED void printPerformanceCounts(const std::map<std::string, InferenceEngine::InferenceEngineProfileInfo>& performanceMap,
                                          std::ostream &stream,
                                          bool bshowHeader = true) {
    long long totalTime = 0;
    // Print performance counts
    if (bshowHeader) {
        stream << std::endl << "performance counts:" << std::endl << std::endl;
    }
    for (const auto & it : performanceMap) {
        std::string toPrint(it.first);
        const int maxLayerName = 30;

        if (it.first.length() >= maxLayerName) {
            toPrint  = it.first.substr(0, maxLayerName - 4);
            toPrint += "...";
        }


        stream << std::setw(maxLayerName) << std::left << toPrint;
        switch (it.second.status) {
        case InferenceEngine::InferenceEngineProfileInfo::EXECUTED:
            stream << std::setw(15) << std::left << "EXECUTED";
            break;
        case InferenceEngine::InferenceEngineProfileInfo::NOT_RUN:
            stream << std::setw(15) << std::left << "NOT_RUN";
            break;
        case InferenceEngine::InferenceEngineProfileInfo::OPTIMIZED_OUT:
            stream << std::setw(15) << std::left << "OPTIMIZED_OUT";
            break;
        }
        stream << std::setw(30) << std::left << "layerType: " + std::string(it.second.layer_type) + " ";
        stream << std::setw(20) << std::left << "realTime: " + std::to_string(it.second.realTime_uSec);
        stream << std::setw(20) << std::left << " cpu: "  + std::to_string(it.second.cpu_uSec);
        stream << " execType: " << it.second.exec_type << std::endl;
        if (it.second.realTime_uSec > 0) {
            totalTime += it.second.realTime_uSec;
        }
    }
    stream << std::setw(20) << std::left << "Total time: " + std::to_string(totalTime) << " microseconds" << std::endl;
}

static UNUSED void printPerformanceCounts(InferenceEngine::InferRequest request, std::ostream &stream) {
    auto perfomanceMap = request.GetPerformanceCounts();
    printPerformanceCounts(perfomanceMap, stream);
}
