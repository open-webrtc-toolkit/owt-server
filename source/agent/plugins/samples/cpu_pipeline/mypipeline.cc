// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <string.h>
#include "toml.hpp"
#include "mypipeline.h"

MyPipeline::MyPipeline() {
    inputwidth = 640;
    inputheight = 480;
    inputframerate = 24;
    pipeline = NULL; 
    source = NULL;
    fakesink = NULL;
    std::cout << "MyPipeline constructor" << std::endl;
}

rvaStatus MyPipeline::PipelineInit(std::unordered_map<std::string, std::string> params) {
    std::cout << "In my plugin init." << std::endl;

    std::unordered_map<std::string,std::string>::const_iterator width = params.find ("inputwidth");
    if ( width == params.end() )
        std::cout << "inputwidth is not found"  << std::endl;
    else
        inputwidth = std::atoi(width->second.c_str());

    std::unordered_map<std::string,std::string>::const_iterator height = params.find ("inputheight");
    if ( height == params.end() )
        std::cout << "inputheight is not found" << std::endl;
    else
        inputheight = std::atoi(height->second.c_str());

    std::unordered_map<std::string,std::string>::const_iterator framerate = params.find ("inputframerate");
    if ( framerate == params.end() )
        std::cout << "inputframerate is not found" << std::endl;
    else
        inputframerate = std::atoi(framerate->second.c_str());

    std::unordered_map<std::string,std::string>::const_iterator name = params.find ("pipelinename");
    if ( name == params.end() )
        std::cout << "inputframerate is not found" << std::endl;
    else
        pipelinename = name->second;

    return RVA_ERR_OK;
}

rvaStatus MyPipeline::PipelineClose() {
    return RVA_ERR_OK;
}

GstElement * MyPipeline::InitializePipeline() {
     /* Initialize GStreamer */
    gst_init(NULL, NULL);

    std::cout << "Start intialize elements" << std::endl;
    /* Create the elements */
    source = gst_element_factory_make("appsrc", "appsource");
    h264parse = gst_element_factory_make("h264parse","parse"); 
    decodebin = gst_element_factory_make("avdec_h264","decode");
    postproc = gst_element_factory_make("videoconvert","postproc");
    detect = gst_element_factory_make("gvadetect","detect"); 
    counter = gst_element_factory_make("gvafpscounter","counter"); 
    fakesink = gst_element_factory_make("filesink","fake");
    watermark = gst_element_factory_make("gvawatermark","rate");
    converter = gst_element_factory_make("videoconvert","convert");
    encoder = gst_element_factory_make("x264enc","encode");
    outsink = gst_element_factory_make("appsink","appsink");

    /* Create the empty VideoGstAnalyzer */
    pipeline = gst_pipeline_new("detect-pipeline");


    if (!detect){
        std::cout << "detect element coule not be created" << std::endl;
        return NULL;
    }

    if (!pipeline || !source || !decodebin || !h264parse || !postproc || !watermark || !outsink) {
        std::cout << "pipeline or source or decodebin or h264parse or postproc elements could not be created." << std::endl;
        return NULL;
    }

    return pipeline;
} 

rvaStatus MyPipeline::LinkElements() {
    gboolean link_ok;
    GstCaps *postprocsrccaps, *postprocsinkcaps;
    const char* path = std::getenv("CONFIGFILE_PATH");
    const auto data = toml::parse(path);
    const auto& pipelineconfig = toml::find(data, pipelinename.c_str());
    const auto model = toml::find<std::string>(pipelineconfig, "modelpath");
    const auto inferencewidth = toml::find<std::int64_t>(pipelineconfig, "inferencewidth");
    const auto inferenceheight = toml::find<std::int64_t>(pipelineconfig, "inferenceheight");
    const auto inferenceframerate = toml::find<std::int64_t>(pipelineconfig, "inferenceframerate");
    const auto device = toml::find<std::string>(pipelineconfig, "device");

    std::cout << "inferencewidth is:" << inferencewidth << std::endl;
    std::cout << "inferenceheight is:" << inferenceheight << std::endl;
    std::cout << "inferenceframerate is:" << inferenceframerate << std::endl;
    std::cout << "model is:" << model << std::endl;
    std::cout << "inputwidth is:" << inputwidth << std::endl;
    std::cout << "input height is:" << inputheight << std::endl;
    std::cout << "input framerate is:" << inputframerate << std::endl;

    postprocsinkcaps = gst_caps_from_string("video/x-raw(memory:VASurface),format=NV12");
    postprocsrccaps = gst_caps_from_string("video/x-raw(memory:VASurface),format=NV12");

    GstCaps* inputcaps = gst_caps_new_simple("video/x-h264",
                "format", G_TYPE_STRING, "avc",
                "width", G_TYPE_INT, inputwidth,
                "height", G_TYPE_INT, inputheight,
                "framerate", GST_TYPE_FRACTION, inputframerate, 1, NULL);

    g_object_set(source, "caps", inputcaps, NULL);
    gst_caps_unref (inputcaps);

    GstCaps* encodecaps = gst_caps_new_simple("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		"width", G_TYPE_INT, 640,
		"height", G_TYPE_INT, 480,
                "profile", G_TYPE_STRING, "constrained-baseline", NULL);

    //g_object_set(G_OBJECT(videorate),"max-rate", inferenceframerate, NULL);
    //g_object_set(G_OBJECT(postproc),"brightness", 0.0001, NULL);
    
    g_object_set(G_OBJECT(encoder),"key-int-max", 30, NULL);
    g_object_set(G_OBJECT(encoder),"pass", 5, NULL);
    g_object_set(G_OBJECT(encoder),"quantizer", 25, NULL);
    g_object_set(G_OBJECT(encoder),"speed-preset", 6, NULL);
    //g_object_set(G_OBJECT(encoder),"cabac", false, NULL);
    g_object_set(G_OBJECT(encoder),"bitrate", 812, NULL);
    //g_object_set(G_OBJECT(encoder),"dct8x8", true, NULL);
    g_object_set(G_OBJECT(encoder),"aud", false, NULL);
    g_object_set(G_OBJECT(encoder),"tune", "zerolatency", NULL);
    
    g_object_set(G_OBJECT(outsink),"drop", true, NULL);

    g_object_set(G_OBJECT(detect),"inference-id", "dtc", NULL);
    g_object_set(G_OBJECT(detect),"device", device.c_str(), NULL);
    g_object_set(G_OBJECT(detect),"model",model.c_str(), NULL);
    g_object_set(G_OBJECT(detect),"cpu-streams", 12, NULL);
    g_object_set(G_OBJECT(detect),"nireq", 24, NULL);
    //g_object_set(G_OBJECT(detect),"pre-proc", "vaapi", NULL);
    //g_object_set(G_OBJECT(detect),"every-nth-frame", 5, NULL);


    gst_bin_add_many(GST_BIN (pipeline), source,decodebin,watermark,postproc,h264parse,detect,converter, encoder,outsink, NULL);

    if (gst_element_link_many(source,h264parse,decodebin, postproc, detect, watermark,converter, encoder, NULL) != TRUE) {
        std::cout << "Elements source,decodebin could not be linked." << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }


    link_ok = gst_element_link_filtered (encoder, outsink, encodecaps);
    gst_caps_unref (encodecaps);

    if (!link_ok) {
        std::cout << "Failed to link videorate and postproc!" << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }

    return RVA_ERR_OK;
} 

// Declare the pipeline 
DECLARE_PIPELINE(MyPipeline)

