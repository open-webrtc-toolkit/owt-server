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

GMainLoop* VideoGstAnalyzer::loop = NULL;

inline bool isH264KeyFrame(uint8_t *data, size_t len)
{
    if (len < 5) {
        //printf("Invalid len %ld\n", len);
        return false;
    }

    if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1) {
        //printf("Invalid start code %d-%d-%d-%d\n", data[0], data[1], data[2], data[3]);
        return false;
    }

    int nal_unit_type = data[4] & 0x1f;

    if (nal_unit_type == 5 || nal_unit_type == 7) {
        //printf("nal_unit_type %d, key_frame %d, len %ld\n", nal_unit_type, true, len);
        return true;
    } else if (nal_unit_type == 9) {
        if (len < 6) {
            //printf("Invalid len %ld\n", len);
            return false;
        }

        int primary_pic_type = (data[5] >> 5) & 0x7;

        //printf("nal_unit_type %d, primary_pic_type %d, key_frame %d, len %ld\n", nal_unit_type, primary_pic_type, (primary_pic_type == 0), len);
        return (primary_pic_type == 0);
    } else {
        //printf("nal_unit_type %d, key_frame %d, len %ld\n", nal_unit_type, false, len);
        return false;
    }
}

inline bool isVp8KeyFrame(uint8_t *data, size_t len)
{
    if (len < 3) {
        //printf("Invalid len %ld\n", len);
        return false;
    }

    unsigned char *c = data;
    unsigned int tmp = (c[2] << 16) | (c[1] << 8) | c[0];

    int key_frame = tmp & 0x1;
    //int version = (tmp >> 1) & 0x7;
    //int show_frame = (tmp >> 4) & 0x1;
    //int first_part_size = (tmp >> 5) & 0x7FFFF;

    //printf("key_frame %d, len %ld\n", (key_frame == 0), len);
    return (key_frame == 0);
}

static void dump(void* index, uint8_t* buf, int len)
{
    char dumpFileName[128];

    snprintf(dumpFileName, 128, "/tmp/analyticsOut-%p", index);
    FILE* bsDumpfp = fopen(dumpFileName, "ab");
    if (bsDumpfp) {
        fwrite(buf, 1, len, bsDumpfp);
        fclose(bsDumpfp);
    }
}


VideoGstAnalyzer::VideoGstAnalyzer(EventRegistry *handle) : m_asyncHandle(handle)
{
    ELOG_INFO("Init");
    sourceid = 0;
    sink = NULL;
    encoder_pad = NULL;
    addlistener = false;
    m_frameCount = 0;
    m_dumpOut = false;
    char* pOut = std::getenv("DUMP_ANALYTICS_OUT");
    if(pOut != NULL) {
        ELOG_INFO("Dump analytics Out stream");
        m_dumpOut = true;
    }
}

VideoGstAnalyzer::~VideoGstAnalyzer()
{
    ELOG_DEBUG("Closed all media in this Analyzer");
    destroyPipeline();
}

bool VideoGstAnalyzer::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    if (m_asyncHandle) {
        return m_asyncHandle->notifyAsyncEvent(event, data);
    } else {
        return false;
    }
}

bool VideoGstAnalyzer::notifyAsyncEventInEmergency(const std::string& event, const std::string& data)
{
    if (m_asyncHandle) {
            return m_asyncHandle->notifyAsyncEventInEmergency(event, data);
        } else {
            return false;
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
        pStreamObj->notifyAsyncEvent("fatal", "GStreamer pipeline error");
        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        ELOG_ERROR("End of stream\n");
        g_main_loop_quit(pStreamObj->loop);
        break;
    case GST_MESSAGE_TAG:{
        GstTagList *tags = NULL;
        gst_message_parse_tag (message, &tags);

        ELOG_DEBUG("Got tags from element %s:\n", GST_OBJECT_NAME (message->src));
        gst_tag_list_unref (tags);
        break;
    }
    case GST_MESSAGE_QOS:{
        ELOG_DEBUG("Got QOS message from %s \n",message->src->name);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:{
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
        ELOG_DEBUG("State change from %d to %d, from:%d \n",old_state, new_state, message->src->name);
        if (strcmp(message->src->name,"appsink") == 0 && new_state == GST_STATE_PLAYING) {
            GstPad *pad = NULL;
            GstCaps *caps = NULL;
            pad = gst_element_get_static_pad (pStreamObj->sink, "sink");
            if (!pad) {
                ELOG_ERROR("Could not retrieve sink pad of sink element\n");
            } else {
                caps = gst_pad_get_current_caps (pad);
                if (!caps)
                caps = gst_pad_query_caps (pad, NULL);

                /* Print and free */
                const gchar *type;
                int width, height;
                std::string str;
                GstStructure *structure;
                structure = gst_caps_get_structure (caps, 0);
                ELOG_DEBUG("Caps for the sink pad type is:%s %s\n",gst_structure_get_name(structure), gst_caps_to_string(caps));
                type = gst_structure_get_name(structure);
                if (strstr(type, "h264") != NULL) {
                    str.append("{\"codec\":\"h264\",\"profile\":\"");
                    str.append(gst_structure_get_string(structure, "profile"));
                    str.append("\",");
                    pStreamObj->outputcodec = "h264";
                } else if (strstr(type, "vp8") != NULL) {
                    str.append("{\"codec\":\"vp8\",");
                    pStreamObj->outputcodec = "vp8";
                }

                gst_structure_get_int (structure, "width", &width);
                gst_structure_get_int (structure, "height", &height);

                str.append("\"width\":");
                str.append(std::to_string(width));
                str.append(",\"height\":");
                str.append(std::to_string(height));
                str.append("}");
                pStreamObj->notifyAsyncEvent("streamadded", str);

                gst_caps_unref (caps);
                gst_object_unref (pad);
            }

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
        gst_object_unref(m_bus);
    }

}

void VideoGstAnalyzer::destroyPipeline()
{
    ELOG_DEBUG("Closed all media in this Analyzer");
    setState(GST_STATE_NULL);
    if (pipeline_ != nullptr && pipelineHandle != nullptr) {
         destroyPlugin(pipeline_);
         dlclose(pipelineHandle);
         stopLoop();
    }
}

bool VideoGstAnalyzer::createPipeline(std::string codec, int width, int height,
    int framerate, int bitrate, int kfi, std::string algo, std::string pluginName)
{
    this->inputcodec = codec;
    this->width = width;
    this->height = height;
    this->framerate = framerate;
    this->bitrate = bitrate;
    this->kfi = kfi;
    this->algo = algo;
    this->libraryName = pluginName;

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
        {"inputcodec", inputcodec},
        {"pipelinename", algo} };
    pipeline_->PipelineConfig(plugin_config_map);

    /* Create the empty VideoGstAnalyzer */
    pipeline = pipeline_->InitializePipeline();

    if (!pipeline) {
        ELOG_ERROR("pipeline Initialization failed\n");
        dlclose(pipelineHandle);
        return false;
    }

    loop = g_main_loop_new(NULL, FALSE);

    m_bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    m_bus_watch_id = gst_bus_add_watch(m_bus, StreamEventCallBack, this);

    rvaStatus status = pipeline_->LinkElements();
    if(status != RVA_ERR_OK) {
       ELOG_ERROR("Link element failed with rvastatus:%d\n",status);
       dlclose(pipelineHandle);
       return false; 
    }
 
    gst_object_unref(m_bus);

    return true;
};

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
    VideoGstAnalyzer* pStreamObj = static_cast<VideoGstAnalyzer*>(data);
    GstSample *sample;
    GstBuffer *buffer;

    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample (GST_APP_SINK (pStreamObj->sink));
    buffer = gst_sample_get_buffer (sample);

    GstMapInfo map;
    gst_buffer_map (buffer, &map, GST_MAP_READ);

    owt_base::Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));

    if (pStreamObj->outputcodec.compare("vp8") == 0) {
        outFrame.format = owt_base::FRAME_FORMAT_VP8;
        outFrame.additionalInfo.video.isKeyFrame = isVp8KeyFrame(map.data, map.size);
    } else if (pStreamObj->outputcodec.find("h264") != std::string::npos) {
        outFrame.format = owt_base::FRAME_FORMAT_H264;
        outFrame.additionalInfo.video.isKeyFrame = isH264KeyFrame(map.data, map.size);
    } else {
        printf("Not support codec:%s\n", pStreamObj->outputcodec.c_str());
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return;
    }

    outFrame.timeStamp = (pStreamObj->m_frameCount++) * 1000 / pStreamObj->framerate * 90;
    outFrame.length = map.size;
    outFrame.additionalInfo.video.width = pStreamObj->width;
    outFrame.additionalInfo.video.height = pStreamObj->height;

    outFrame.payload = map.data;

    pStreamObj->m_gstinternalout->onFrame(outFrame);
    if(pStreamObj->m_dumpOut) {
        dump(pStreamObj, map.data, map.size);
    }

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
}


void VideoGstAnalyzer::stopLoop()
{
    if(loop){
        ELOG_DEBUG("main loop quit\n");
        g_main_loop_quit(loop);
    }
}

void VideoGstAnalyzer::main_loop_thread(gpointer data)
{
    g_main_loop_run(loop);
    g_thread_exit(0);
}

void VideoGstAnalyzer::setState(GstState newstate)
{
    ret = gst_element_set_state(pipeline, newstate);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ELOG_ERROR("Unable to set the pipeline to the PLAYING state.\n");
        gst_object_unref(pipeline);
    }
} 


void VideoGstAnalyzer::setPlaying()
{

    setState(GST_STATE_PLAYING);
    m_thread = g_thread_create((GThreadFunc)main_loop_thread,NULL,TRUE,NULL);
}

bool VideoGstAnalyzer::linkInput(owt_base::FrameSource* videosource) {
    assert(videosource);

    source = gst_bin_get_by_name (GST_BIN (pipeline), "appsource");
    if (!source) {
        ELOG_ERROR("appsrc in pipeline is not created\n");
        return false;
    }

    m_internalin.reset(new GstInternalIn((GstAppSrc*)source, this->framerate));

    sink = gst_bin_get_by_name (GST_BIN (pipeline), "appsink");
    if (sink != nullptr){
        if(m_gstinternalout == nullptr) {
            m_gstinternalout.reset(new GstInternalOut());
        }

        if(encoder_pad == nullptr) {
            GstElement *encoder = gst_bin_get_by_name (GST_BIN (pipeline), "encoder");
            encoder_pad = gst_element_get_static_pad(encoder, "src");
            m_gstinternalout->setPad(encoder_pad);
        }

        if(!addlistener) {
            g_object_set (G_OBJECT (sink), "emit-signals", TRUE, "sync", FALSE, NULL);
            g_signal_connect (sink, "new-sample", G_CALLBACK (new_sample_from_sink), this);
            addlistener = true;
        }
    } else {
        ELOG_ERROR("There is no appsink in pipeline\n");
    }

    g_signal_connect (source, "need-data", G_CALLBACK (start_feed), this);
    g_signal_connect (source, "enough-data", G_CALLBACK (stop_feed), this);
    videosource->addVideoDestination(m_internalin.get());

    setPlaying();
    return true;
}

bool VideoGstAnalyzer::addOutput(owt_base::FrameDestination* out)
{
    ELOG_DEBUG("Add analyzed stream back to OWT\n");
    if (sink != nullptr){
        m_gstinternalout->addVideoDestination(out);
        return true;
    } else {
        ELOG_ERROR("No appsink in pipeline\n");
        return false;
    }
    
}

void VideoGstAnalyzer::removeOutput(owt_base::FrameDestination* out)
{
    ELOG_DEBUG("Disconnect remote connection\n");
    m_gstinternalout->removeVideoDestination(out);
}

}
