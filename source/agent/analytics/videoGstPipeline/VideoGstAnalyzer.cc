// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <zconf.h>
#include "VideoGstAnalyzer.h"
#include <iostream>
#include <string>
#include <unistd.h>

namespace mcu {

DEFINE_LOGGER(VideoGstAnalyzer, "mcu.VideoGstAnalyzer");

GMainLoop* VideoGstAnalyzer::loop = g_main_loop_new(NULL,FALSE);
#define DELAY_VALUE 1000000

VideoGstAnalyzer::VideoGstAnalyzer() {
    ELOG_INFO("Init");
    sourceid = 0;
}

VideoGstAnalyzer::~VideoGstAnalyzer() {
    ELOG_DEBUG("CloseAll");

    ELOG_DEBUG("Closed all media in this Analyzer");
}

void VideoGstAnalyzer::print_one_tag (const GstTagList * list, const gchar * tag, gpointer user_data)
{
  int i, num;

  num = gst_tag_list_get_tag_size (list, tag);
  for (i = 0; i < num; ++i) {
    const GValue *val;

    /* Note: when looking for specific tags, use the gst_tag_list_get_xyz() API,
     * we only use the GValue approach here because it is more generic */
    val = gst_tag_list_get_value_index (list, tag, i);
    if (G_VALUE_HOLDS_STRING (val)) {
      ELOG_DEBUG ("\t%20s : %s\n", tag, g_value_get_string (val));
    } else if (G_VALUE_HOLDS_UINT (val)) {
      ELOG_DEBUG ("\t%20s : %u\n", tag, g_value_get_uint (val));
    } else if (G_VALUE_HOLDS_DOUBLE (val)) {
      ELOG_DEBUG ("\t%20s : %g\n", tag, g_value_get_double (val));
    } else if (G_VALUE_HOLDS_BOOLEAN (val)) {
      ELOG_DEBUG ("\t%20s : %s\n", tag,
          (g_value_get_boolean (val)) ? "true" : "false");
    } else if (GST_VALUE_HOLDS_BUFFER (val)) {
      GstBuffer *buf = gst_value_get_buffer (val);
      guint buffer_size = gst_buffer_get_size (buf);

      ELOG_DEBUG ("\t%20s : buffer of size %u\n", tag, buffer_size);
    } else if (GST_VALUE_HOLDS_DATE_TIME (val)) {
      
      ELOG_DEBUG ("\t%20s : hold date time\n", tag);

    } else {
      ELOG_DEBUG ("\t%20s : tag of type '%s'\n", tag, G_VALUE_TYPE_NAME (val));
    }
  }
}

gboolean VideoGstAnalyzer::StreamEventCallBack(GstBus *bus, GstMessage *message, gpointer data)
    {
        ELOG_DEBUG("Got %s message\n", GST_MESSAGE_TYPE_NAME(message));
 
        VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
 
        switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(message, &err, &debug);
            ELOG_ERROR("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            g_main_loop_quit(pStreamObj->loop);
    
            break;
        }
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            ELOG_ERROR("End of stream\n");
            g_main_loop_quit(pStreamObj->loop);
            break;
        case GST_MESSAGE_TAG:{
            /* end-of-stream */
            GstTagList *tags = NULL;
            gst_message_parse_tag (message, &tags);

            ELOG_DEBUG("Got tags from element %s:\n", GST_OBJECT_NAME (message->src));
            gst_tag_list_foreach (tags, print_one_tag, NULL);
            ELOG_DEBUG("\n");
            gst_tag_list_unref (tags);
            break;
        }
        case GST_MESSAGE_QOS:{
            /* end-of-stream */
            ELOG_DEBUG("Got QOS message from %s \n",message->src->name);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:{
            ELOG_DEBUG("State change\n");
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
            ELOG_DEBUG("State change from %d to %d, play:%d \n",old_state, new_state, GST_STATE_PAUSED);
            if(new_state == GST_STATE_PLAYING) {
                ELOG_DEBUG("Create debug dot file\n");
                //GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pStreamObj->pipeline),GST_DEBUG_GRAPH_SHOW_ALL, "play");
            }
            break;
        }
        default:
            /* unhandled message */
            break;
        }
        return true;
    }

void VideoGstAnalyzer::clearPipeline()
    {
        if (pipeline != nullptr){
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(GST_OBJECT(pipeline));
            gst_object_unref(m_bus);
            g_source_remove(m_bus_watch_id);
            g_main_loop_unref(loop);
        }
 
    }

int VideoGstAnalyzer::createPipeline() {
    /* Initialize GStreamer */
    gst_init(NULL, NULL);

    /* Create the elements */
    source = gst_element_factory_make("appsrc", "appsource");
    h264parse = gst_element_factory_make("h264parse","parse"); 
    decodebin = gst_element_factory_make("vaapih264dec","decode");
    postproc = gst_element_factory_make("vaapipostproc","postproc");
    detect = gst_element_factory_make("gvadetect","detect"); 
    fpssink = gst_element_factory_make("fakesink","sendsink");
    fakesink = gst_element_factory_make("fpsdisplaysink","fake");
    videosink = gst_element_factory_make("fakesink","video");
    sendsink = gst_element_factory_make("sendsink","send");
    videorate = gst_element_factory_make("videorate","rate");

    loop = g_main_loop_new(NULL, FALSE);

    /* Create the empty VideoGstAnalyzer */
    pipeline = gst_pipeline_new("test-pipeline");

    if (!fpssink) {
        ELOG_ERROR("fpssink element coule not be created\n");
        return -1;
    }

    if (!detect){
        ELOG_ERROR("detect element coule not be created\n");
        return -1;
    }

    if (!pipeline || !source || !decodebin || !h264parse || !postproc || !videorate) {
        ELOG_ERROR("pipeline or source or decodebin or h264parse or postproc elements could not be created.\n");
        return -1;
    }

    m_bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    m_bus_watch_id = gst_bus_add_watch(m_bus, StreamEventCallBack, this);
 
    gst_object_unref(m_bus);

    return 0;
};

void VideoGstAnalyzer::pad_added_handler(GstElement *src, GstPad *new_pad, GstElement *data){
    GstPad *sink_pad = gst_element_get_static_pad(data, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    ELOG_DEBUG("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    /*if (gst_pad_is_linked(sink_pad)) {
        g_print("We are already linked. Ignoring.\n");
        goto exit;
    }*/
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);

    ret = gst_pad_link(new_pad,sink_pad);
    if(GST_PAD_LINK_FAILED(ret)){
        ELOG_ERROR("Type is '%s' but link failed.\n",new_pad_type);
        if(new_pad_caps != NULL)
        gst_caps_unref(new_pad_caps);

        gst_object_unref(sink_pad);
    }
    else{
        ELOG_DEBUG("Link succeeded(type '%s').\n",new_pad_type);
    }
    
}

void VideoGstAnalyzer::on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;
    /* We can now link this pad with the rtsp-decoder sink pad */
    ELOG_DEBUG ("Dynamic pad created, linking source/demuxer\n");
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
}

gboolean VideoGstAnalyzer::push_data (gpointer data) {
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    pStreamObj->m_internalin->setPushData(true);
    return true;
}

void VideoGstAnalyzer::start_feed (GstElement * source, guint size, gpointer data)
{
    #if 1
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    pStreamObj->m_internalin->setPushData(true);
    #else
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    if (pStreamObj->sourceid == 0) {
        ELOG_INFO("Start feeding data\n");
        pStreamObj->sourceid = g_idle_add ((GSourceFunc) push_data, data);
    }
    #endif
}

void VideoGstAnalyzer::stop_feed (GstElement * source, gpointer data)
{
    #if 1
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    pStreamObj->m_internalin->setPushData(false);
    #else
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    if (pStreamObj->sourceid != 0) {
        ELOG_INFO("Stop feeding data\n");
        g_source_remove (pStreamObj->sourceid);
        pStreamObj->m_internalin->setPushData(false);
        pStreamObj->sourceid = 0;
    } 
    #endif 
    
}

int VideoGstAnalyzer::addElementMany() {

    gboolean link_ok;
    GstCaps *caps, *ratecaps;

    caps = gst_caps_new_simple ("video/x-raw",
          "format", G_TYPE_STRING, "BGRA",
          "width", G_TYPE_INT, 672,
          "height", G_TYPE_INT, 384,
          NULL);

    ratecaps = gst_caps_new_simple ("video/x-raw",
          "framerate", GST_TYPE_FRACTION, 5, 1,
          NULL);

    GstCaps* incaps = gst_caps_new_simple("video/x-h264",
                "format", G_TYPE_STRING, "avc",
                "width", G_TYPE_INT, width,
                "height", G_TYPE_INT, height,
                "framerate", GST_TYPE_FRACTION, framerate, 1, NULL);

    g_object_set(source, "caps", incaps, NULL);
    gst_caps_unref (incaps);

    g_object_set(G_OBJECT(detect),"inference-id", "dtc", NULL);
    g_object_set(G_OBJECT(detect),"device", "HDDL", NULL);
    g_object_set(G_OBJECT(detect),"model","/home/models/pedestrian-detection-adas-0002-fp16.xml", NULL);
    //g_object_set(G_OBJECT(detect),"every-nth-frame", 5, NULL);

    
    g_object_set(G_OBJECT(fakesink),"text-overlay", false, NULL);
    g_object_set(G_OBJECT(fakesink),"signal-fps-measurements", true, NULL);
    g_object_set(G_OBJECT(fakesink),"sync", false, NULL);
    g_object_set(G_OBJECT(fakesink),"video-sink", videosink, NULL);

    ELOG_DEBUG("=================Set fakedisplaysink properties \n");

    #if 0
    gst_bin_add_many(GST_BIN (pipeline), source,fpssink, NULL);

    if (gst_element_link_many(source,fpssink,NULL) != TRUE) {
    //if (gst_element_link_many(source,receive,h264parse,decodebin,postproc,NULL) != TRUE) {
        ELOG_ERROR("Elements source,decodebin could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_signal_connect (source, "need-data", G_CALLBACK (start_feed), this);
    g_signal_connect (source, "enough-data", G_CALLBACK (stop_feed), this);

    #else
    gst_bin_add_many(GST_BIN (pipeline), source,decodebin,videorate,postproc,h264parse,detect,fakesink, NULL);


    if (gst_element_link_many(source,h264parse,decodebin,videorate,NULL) != TRUE) {
    //if (gst_element_link_many(source,receive,h264parse,decodebin,postproc,NULL) != TRUE) {
        ELOG_ERROR("Elements source,decodebin could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_signal_connect (source, "need-data", G_CALLBACK (start_feed), this);
    g_signal_connect (source, "enough-data", G_CALLBACK (stop_feed), this);

    link_ok = gst_element_link_filtered (videorate, postproc, ratecaps);
    gst_caps_unref (ratecaps);

      if (!link_ok) {
        ELOG_ERROR ("Failed to link videorate and postproc!");
        gst_object_unref(pipeline);
        return -1;
      }

   link_ok = gst_element_link_filtered (postproc, detect, caps);
    gst_caps_unref (caps);

      if (!link_ok) {
        ELOG_ERROR ("Failed to link postproc and detect!");
        gst_object_unref(pipeline);
        return -1;
      }

    if(gst_element_link(detect,fakesink) !=TRUE){
        ELOG_ERROR("Elements detect,fakesink could not be linked. \n");
        gst_object_unref(pipeline);
        return -1;
    }

    #endif

    return 0;
}


void VideoGstAnalyzer::stopLoop(){
    if(loop){
        ELOG_DEBUG("main loop quit\n");
        g_main_loop_quit(loop);
    }
    g_thread_join(m_thread);
}

void VideoGstAnalyzer::disconnect(int connectionID){
    printf("Disconnect remote connection\n");
}

void VideoGstAnalyzer::main_loop_thread(gpointer data){
    g_main_loop_run(loop);
    g_thread_exit(0);
}

void VideoGstAnalyzer::setState() {
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ELOG_ERROR("Unable to set the pipeline to the PLAYING state.\n");
        gst_object_unref(pipeline);
    }
} 


void VideoGstAnalyzer::printFPS() {
    gchar *fps_msg;
    guint delay_show_FPS = 0;
    while(1) {
        g_object_get (G_OBJECT (fakesink), "last-message", &fps_msg, NULL);
        if (fps_msg != NULL) {
            ELOG_INFO("Frame info: %s\n", fps_msg);
        }

        boost::this_thread::sleep( boost::posix_time::milliseconds(200) );
    }
} 

int VideoGstAnalyzer::setPlaying() {

    m_playthread = boost::thread(boost::bind(&VideoGstAnalyzer::setState, this));

    //boost::thread(boost::bind(&VideoGstAnalyzer::printFPS, this));

    m_thread = g_thread_create((GThreadFunc)main_loop_thread,NULL,TRUE,NULL);

    return 0;
}

void VideoGstAnalyzer::emitListenTo(int minPort, int maxPort) {
    pthread_t tid;
    tid = pthread_self();
    ELOG_DEBUG("*****main thread tid: %u (0x%x)\n", (unsigned int)tid, (unsigned int)tid);
    m_internalin.reset(new InternalIn((GstAppSrc*)source, minPort, maxPort));
    
}

void VideoGstAnalyzer::emitConnectTo(int connectionID, char* ip, int remotePort){
    ELOG_DEBUG("============Connect to remote port\n");
}

int VideoGstAnalyzer::getListeningPort() {
    int listeningPort; 
    listeningPort = m_internalin->getListeningPort();
    ELOG_DEBUG(">>>>>Listen port is :%d\n", listeningPort);
    return listeningPort; 
}

void VideoGstAnalyzer::setOutputParam(std::string codec, int width, int height, 
    int framerate, int bitrate, int kfi, std::string algo, std::string pluginName){

    this->codec = codec;
    this->width = width;
    this->height = height;
    this->framerate = framerate;
    this->bitrate = bitrate;
    this->kfi = kfi;
    this->algo = algo;
    this->pluginName = pluginName;

    std::cout<<"setting param,codec="<<this->codec<<",width="<<this->width<<",height="
        <<this->height<<",framerate="<<this->framerate<<",bitrate="<<this->bitrate<<",kfi="
        <<this->kfi<<",algo="<<this->algo<<",pluginName="<<this->pluginName<<std::endl;
}

}
