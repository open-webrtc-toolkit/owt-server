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

rvaStatus MyPipeline::PipelineConfig(std::unordered_map<std::string, std::string> params) {
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
    if(!source) {
        std::cout << "source element is not created" << std::endl;
        return NULL;
    }
    fakesink = gst_element_factory_make("filesink","fake");
    if(!fakesink) {
        std::cout << "fakesink element is not created" << std::endl;
        return NULL;
    }

    pipeline = gst_pipeline_new("sample-pipeline");


    if (!pipeline ) {
        std::cout << "pipeline  could not be created." << std::endl;
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
    const std::string outputpath = toml::find<std::string>(pipelineconfig, "outputpath");

    std::cout << "outputpath is:" << outputpath << std::endl;
    std::cout << "inputwidth is:" << inputwidth << std::endl;
    std::cout << "input height is:" << inputheight << std::endl;
    std::cout << "input framerate is:" << inputframerate << std::endl;

    GstCaps* inputcaps = gst_caps_new_simple("video/x-h264",
                "format", G_TYPE_STRING, "avc",
                "width", G_TYPE_INT, inputwidth,
                "height", G_TYPE_INT,inputheight, NULL);
                //"framerate", GST_TYPE_FRACTION, inputframerate, 1, NULL);

    std::cout << "Set input caps to source element" << std::endl;
    if(!source) {
        std::cout << "source element is not created in Link elements" << std::endl;
        return RVA_ERR_LINK;
    }

    if(!fakesink) {
        std::cout << "fakesink element is not created in Link elements" << std::endl;
        return RVA_ERR_LINK;
    }

    g_object_set(G_OBJECT(source), "caps", inputcaps, NULL);
    gst_caps_unref (inputcaps);
    std::cout << "Set output path" << std::endl;
    g_object_set(G_OBJECT(fakesink),"location", outputpath.c_str(), NULL);

    
    std::cout << "Add elements" << std::endl;

    gst_bin_add_many(GST_BIN (pipeline), source,fakesink, NULL);

    std::cout << "Link elements" << std::endl;
    if(gst_element_link(source,fakesink) !=TRUE){
        std::cout << "Elements detect,fakesink could not be linked. " << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }


    return RVA_ERR_OK;
} 


// Declare the pipeline 
DECLARE_PIPELINE(MyPipeline)

