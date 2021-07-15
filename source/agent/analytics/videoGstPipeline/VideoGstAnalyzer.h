// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoGstAnalyzer_H
#define VideoGstAnalyzer_H

#include <gst/gst.h>
#include <glib-object.h>
#include <gst/pbutils/encoding-profile.h>
#include <gst/app/gstappsink.h>
#include <string>
#include <boost/thread.hpp>
#include <logger.h>
#include "GstInternalIn.h"
#include "GstInternalOut.h"
#include "AnalyticsPipeline.h"
#include <stdio.h>
#include "MediaFramePipeline.h"
#include <EventRegistry.h>

namespace mcu {

class VideoGstAnalyzer : public EventRegistry {
    DECLARE_LOGGER();
public:
    VideoGstAnalyzer(EventRegistry* handle);
    ~VideoGstAnalyzer();
    bool createPipeline(std::string codec, int width, int height,
    int framerate, int bitrate, int kfi, std::string algo, std::string pluginName);
    void clearPipeline();
    void removeOutput(owt_base::FrameDestination* out);
    bool addOutput(owt_base::FrameDestination* out);
    bool linkInput(owt_base::FrameSource* videosource);

    static void start_feed (GstElement * source, guint size, gpointer data);
    static gboolean push_data (gpointer data);
    static void stop_feed (GstElement * source, gpointer data);
    static void new_sample_from_sink (GstElement * source, gpointer data);

protected:
    static void main_loop_thread(gpointer);
    static GMainLoop *loop;
    static gboolean StreamEventCallBack(GstBus *bus, GstMessage *message, gpointer data);
    void setState(GstState newstate);
    void setPlaying();
    void stopLoop();
    void destroyPipeline();
    // EventRegistry
    bool notifyAsyncEvent(const std::string& event, const std::string& data) override;
    bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data) override;

    boost::scoped_ptr<GstInternalIn> m_internalin;
    boost::scoped_ptr<GstInternalOut> m_gstinternalout;
    guint sourceid;

private:
    GstElement *pipeline, *source, *sink;
    GstPad *encoder_pad;
    void* pipelineHandle;
    rvaPipeline* pipeline_;
    rva_create_t* createPlugin;
    rva_destroy_t* destroyPlugin;
    EventRegistry *m_asyncHandle;
    
    GstStateChangeReturn ret;

    GThread *m_thread;
    guint m_bus_watch_id;
    GstBus *m_bus;
    boost::thread m_playthread;

    int connectPort;
    int m_frameCount;

    //param
    std::string inputcodec;
    std::string outputcodec;
    std::string algo,libraryName;
    std::string resolution;
    int width,height;
    int framerate,bitrate;
    int kfi; //keyFrameInterval
    bool addlistener;
    bool m_dumpOut;
};

}

#endif //VideoGstAnalyzer_H
