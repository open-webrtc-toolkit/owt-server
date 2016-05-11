/*
 * Copyright 2016 Intel Corporation All Rights Reserved.
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
#include "VideoDisplay.h"

#include <boost/thread/shared_mutex.hpp>
#include <fcntl.h>
#include <logger.h>
#include <sys/stat.h>
#include <va/va_drm.h>

struct VADisplayDeleter
{
    VADisplayDeleter(int fd):m_fd(fd) {}
    void operator()(VADisplay* display)
    {
        vaTerminate(*display);
        delete display;
        close(m_fd);
    }
private:
    int m_fd;
};

class DisplayGetter {
public:
    static boost::shared_ptr<VADisplay> GetVADisplay()
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        boost::shared_ptr<VADisplay> display = m_display;
        if (display)
            return display;
        ELOG_ERROR("create new display");
        int fd = open("/dev/dri/renderD128", O_RDWR);
        if (fd < 0)
            fd = open("/dev/dri/card0", O_RDWR);
        if (fd < 0) {
            ELOG_ERROR("can't open drm device");
            return display;
        }
        VADisplay vadisplay = vaGetDisplayDRM(fd);
        int majorVersion, minorVersion;
        VAStatus vaStatus = vaInitialize(vadisplay, &majorVersion, &minorVersion);
        if (vaStatus != VA_STATUS_SUCCESS) {
            ELOG_ERROR("va init failed, status =  %d", vaStatus);
            close(fd);
            return display;
        }
        display.reset(new VADisplay(vadisplay), VADisplayDeleter(fd));
        m_display = display;
        return display;
    }

private:
    static boost::shared_mutex m_mutex;
    //TODO: change this to weak ptr
    static boost::shared_ptr<VADisplay> m_display;

    DECLARE_LOGGER();
};

DEFINE_LOGGER(DisplayGetter, "mcu.media.DisplayGetter");

boost::shared_mutex DisplayGetter::m_mutex;
boost::shared_ptr<VADisplay> DisplayGetter::m_display;

boost::shared_ptr<VADisplay> GetVADisplay()
{
    return DisplayGetter::GetVADisplay();
}

