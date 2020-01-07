// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <zconf.h>
#include <dlfcn.h>
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
    sink = NULL;
    fp = NULL;
    encoder_pad = NULL;
}

VideoGstAnalyzer::~VideoGstAnalyzer() {
    ELOG_DEBUG("Closed all media in this Analyzer");
    if (pipeline_ != nullptr && pipelineHandle != nullptr) {
         destroyPlugin(pipeline_);
         dlclose(pipelineHandle);
    }
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
            if(new_state == GST_STATE_PLAYING || new_state == GST_STATE_PAUSED) {
                ELOG_DEBUG("Create debug dot file\n");
                GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pStreamObj->pipeline),GST_DEBUG_GRAPH_SHOW_ALL, "play");
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
            g_source_remove(m_bus_watch_id);
            g_main_loop_unref(loop);
        }
 
    }

int VideoGstAnalyzer::createPipeline() {

    loop = g_main_loop_new(NULL, FALSE);

    pipelineHandle = dlopen(libraryName.c_str(), RTLD_LAZY);
    if (pipelineHandle == nullptr) {
        ELOG_ERROR_T("Failed to open the plugin.(%s)", libraryName.c_str());
        return false;
    }

    createPlugin = (rva_create_t*)dlsym(pipelineHandle, "CreatePipeline");
    destroyPlugin = (rva_destroy_t*)dlsym(pipelineHandle, "DestroyPipeline");

    if (createPlugin == nullptr || destroyPlugin == nullptr) {
        ELOG_ERROR_T("Failed to get plugin interface.");
        dlclose(pipelineHandle);
        return false;
    }

    pipeline_ = createPlugin();
    if (pipeline_ == nullptr) {
        ELOG_ERROR_T("Failed to create the plugin.");
        dlclose(pipelineHandle);
        return false;
    }

    std::unordered_map<std::string, std::string> plugin_config_map = {
        {"inputwidth", std::to_string(width)},
        {"inputheight", std::to_string(height)},
        {"inputframerate", std::to_string(framerate)},
        {"pipelinename", algo} };
    pipeline_->PipelineInit(plugin_config_map);

    /* Create the empty VideoGstAnalyzer */
    pipeline = pipeline_->InitializePipeline();

    if (!pipeline) {
        ELOG_ERROR("pipeline could not be created\n");
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
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    pStreamObj->m_internalin->setPushData(true);
}

void VideoGstAnalyzer::stop_feed (GstElement * source, gpointer data)
{
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    pStreamObj->m_internalin->setPushData(false);
    
}

void VideoGstAnalyzer::new_sample_from_sink (GstElement * source, gpointer data)
{
    ELOG_DEBUG("Got new sample from sink\n");
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    GstSample *sample;
    GstBuffer *app_buffer, *buffer;

    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample (GST_APP_SINK (pStreamObj->sink));
    buffer = gst_sample_get_buffer (sample);

    /* make a copy */
    //app_buffer = gst_buffer_copy (buffer);
    GstMapInfo map;
    gst_buffer_map (buffer, &map, GST_MAP_READ);
 
    /* we don't need the appsink sample anymore */
    
    for ( auto& x: pStreamObj->m_internalout)
        x->onFrame(map.data, map.size);

    /*if(pStreamObj->fp == NULL) 
        pStreamObj->fp = fopen("/tmp/receive","w+b");
    fwrite(map.data,sizeof(char),map.size,pStreamObj->fp);*/

    gst_sample_unref(sample);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}

int VideoGstAnalyzer::addElementMany() {
    if(pipeline_)
        pipeline_->LinkElements();

    source = gst_bin_get_by_name (GST_BIN (pipeline), "appsource");
    if (!source) {
        ELOG_ERROR("appsrc in pipeline does not be created\n");
        return -1;
    }

    sink = gst_bin_get_by_name (GST_BIN (pipeline), "appsink");
    if (!sink) {
        ELOG_ERROR("There is no appsink in pipeline\n");
    }

    g_signal_connect (source, "need-data", G_CALLBACK (start_feed), this);
    g_signal_connect (source, "enough-data", G_CALLBACK (stop_feed), this);

    return 0;
}


void VideoGstAnalyzer::stopLoop(){
    if(loop){
        ELOG_DEBUG("main loop quit\n");
        g_main_loop_quit(loop);
    }
    g_thread_join(m_thread);
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


int VideoGstAnalyzer::setPlaying() {

    m_playthread = boost::thread(boost::bind(&VideoGstAnalyzer::setState, this));

    m_thread = g_thread_create((GThreadFunc)main_loop_thread,NULL,TRUE,NULL);

    return 0;
}

void VideoGstAnalyzer::emitListenTo(int minPort, int maxPort) {
    ELOG_DEBUG("Listening\n");
    m_internalin.reset(new InternalIn((GstAppSrc*)source, minPort, maxPort));
    
}

void VideoGstAnalyzer::emitConnectTo(int connectionID, char* ip, int remotePort){
    if (sink != nullptr){
        ELOG_DEBUG("Connect to remote ip %s port %d with connetionID %d\n", ip, remotePort);
        
    }
    else {
        ELOG_ERROR("No appsink in the pipeline\n");
    }
}

void VideoGstAnalyzer::addOutput(int connectionID, owt_base::InternalOut* out) {
    ELOG_DEBUG("Add analyzed stream back to OWT\n");
    if (sink != nullptr){
        //m_internalout.insert(connectionID, out);
        //boost::unique_lock<boost::shared_mutex> lock(m_video_dests_mutex);
        if(encoder_pad == nullptr) {
            GstElement *encoder = gst_bin_get_by_name (GST_BIN (pipeline), "encoder");
            encoder_pad = gst_element_get_static_pad(encoder, "src");
            out->setPad(encoder_pad);
            ELOG_ERROR("Set encoder pad to internal output\n");
        }
        m_internalout.push_back(out);
        //lock.unlock();
        g_object_set (G_OBJECT (sink), "emit-signals", TRUE, "sync", FALSE, NULL);
        g_signal_connect (sink, "new-sample", G_CALLBACK (new_sample_from_sink), this);
    } else {
        ELOG_ERROR("No appsink in pipeline\n");
    }
    
}

void VideoGstAnalyzer::disconnect(owt_base::InternalOut* out){
    ELOG_DEBUG("Disconnect remote connection\n");
    m_internalout.remove(out);
    g_object_set (G_OBJECT (sink), "emit-signals", FALSE, "sync", FALSE, NULL);
}

int VideoGstAnalyzer::getListeningPort() {
    int listeningPort; 
    listeningPort = m_internalin->getListeningPort();
    ELOG_DEBUG(">>>>>Listen port is :%d\n", listeningPort);
    return listeningPort; 
}

void VideoGstAnalyzer::setOutputParam(std::string codec, int width, int height, 
    int framerate, int bitrate, int kfi, std::string algo, std::string libraryName){

    this->codec = codec;
    this->width = width;
    this->height = height;
    this->framerate = framerate;
    this->bitrate = bitrate;
    this->kfi = kfi;
    this->algo = algo;
    this->libraryName = libraryName;

    std::cout << "setting param,codec=" << this->codec<<",width=" << this->width << ",height="
         << this->height << ",framerate=" << this->framerate<<",bitrate=" << this->bitrate << ",kfi="
         << this->kfi << ",algo=" << this->algo << ",pluginName=" << this->libraryName << std::endl;
}

}
