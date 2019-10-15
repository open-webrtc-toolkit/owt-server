#include "ResourceUtil.h"
#include <iostream>
#include <string>
#include <vector>

using namespace InferenceEngine;

namespace mcu {

DEFINE_LOGGER(ResourceUtil, "mcu.media.ResourceUtil");

ResourceUtil::ResourceUtil() {
    //m_hddlClient = new hddl::HddlClient("HDDLMetric");
}

ResourceUtil::~ResourceUtil() {
    //delete m_hddlClient;
}

float ResourceUtil::getVPUUtil() { 

    #if 0
    if (m_hddlClient->checkInitStatus() != hddl::HDDL_OK) {
        ELOG_ERROR("Error: checkInitStatus failed.\n");
        return -1;
    }

    hddl::HddlQuery query;
    hddl::QueryType type = hddl::QUERY_TYPE_ALL;
    hddl::HddlStatusCode hddlStatus = m_hddlClient->query(type, &query);
    if (hddlStatus < 0) {
        ELOG_ERROR("Error: Query device info error, retCode= %d \n", hddlStatus);
        return -1;
    }
    //query.dump();

    std::vector<float> v = query.getDeviceUtilization();
    float sum=0, avg;
    for(float n : v) {
            sum +=n;
    }
    avg = sum/v.size();
    #else
    //Core ie;
    std::vector<float> v = ie.GetMetric("HDDL", VPU_HDDL_METRIC(DEVICE_UTILIZATION));
    float sum=0, avg=0;
    for(float n : v) {
            sum +=n;
    }
    avg = sum/v.size();
    #endif

    return avg;
}

}
