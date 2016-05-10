/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifndef EventRegistry_h
#define EventRegistry_h

#include <string>

class EventRegistry {
public:
    virtual ~EventRegistry() {}
    // `notifyAsyncEvent()' sends a notification to the event handler, which would be handled with FIFO rule.
    virtual bool notifyAsyncEvent(const std::string& event, const std::string& data) = 0;
    // In contrast to `notifyAsyncEvent()', `notifyAsyncEventInEmergency()' sends a notification to the event handler,
    // which would be handled before other normal notifications (LIFO).
    // Do not abuse it.
    virtual bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data) = 0;
};

#endif // EventRegistry_h
