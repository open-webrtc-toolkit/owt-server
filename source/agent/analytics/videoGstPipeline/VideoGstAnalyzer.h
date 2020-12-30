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

namespace mcu {

class VideoGstAnalyzer{
    DECLARE_LOGGER();
public:
    VideoGstAnalyzer();
    virtual ~VideoGstAnalyzer();
    int createPipeline();
    void clearPipeline();
    int getListeningPort();
    void emitListenTo(int minPort,int maxPort);
    int setPlaying();

    int addElementMany();

    void setOutputParam(std::string codec, int width, int height, 
    int framerate, int bitrate, int kfi, std::string algo, std::string pluginName);

    void stopLoop();

    void disconnect(owt_base::FrameDestination* out);

    void addOutput(int connectionID, owt_base::FrameDestination* out);

    static void pad_added_handler(GstElement *src, GstPad *new_pad, GstElement *data);
    static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);

    static void start_feed (GstElement * source, guint size, gpointer data);
    static gboolean push_data (gpointer data);
    static void stop_feed (GstElement * source, gpointer data);
    static void new_sample_from_sink (GstElement * source, gpointer data);

protected:
    static void main_loop_thread(gpointer);
    static GMainLoop *loop;
    static gboolean StreamEventCallBack(GstBus *bus, GstMessage *message, gpointer data);
    void setState(GstState newstate);
    boost::scoped_ptr<GstInternalIn> m_internalin;
    boost::scoped_ptr<GstInternalOut> m_gstinternalout;
    //std::list<owt_base::InternalOut*> m_internalout;
    guint sourceid;

private:
    GstElement *pipeline, *source, *sink;
    GstPad *encoder_pad;
    void* pipelineHandle;
    rvaPipeline* pipeline_;
    rva_create_t* createPlugin;
    rva_destroy_t* destroyPlugin;
    
    GstStateChangeReturn ret;

    GThread *m_thread;
    guint m_bus_watch_id;
    GstBus *m_bus;
    boost::thread m_playthread;

    int connectPort;
    int m_frameCount;

    //param
    std::string codec;
    std::string algo,libraryName;
    std::string resolution;
    int width,height;
    int framerate,bitrate;
    int kfi; //keyFrameInterval
    bool addlistener;
};

}

#endif //VideoGstAnalyzer_H
