// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MYPIPELINE_H
#define MYPIPELINE_H

#include <gst/gst.h>
#include <boost/thread.hpp>
#include "pipeline.h"

// Class definition for the pipeline invoked by the sample service.
class MyPipeline : public rvaPipeline {
public:
    MyPipeline();

    virtual rvaStatus PipelineInit(std::unordered_map<std::string, std::string> params);

    virtual rvaStatus PipelineClose();

    virtual rvaStatus GetPipelineParams(std::unordered_map<std::string, std::string>& params) {
        return RVA_ERR_OK;
    }

    virtual rvaStatus SetPipelineParams(std::unordered_map<std::string, std::string> params) {
        return RVA_ERR_OK;
    }

    virtual GstElement * InitializePipeline();

    virtual rvaStatus LinkElements();

protected:
    void printFPS();

private:
    GstElement *pipeline, *source, *receive,*detect,*decodebin,*postproc,*h264parse,*videosink,*fakesink,*sendsink, *rtsph264, *videorate;
    int inputwidth, inputheight, inputframerate;
    std::string pipelinename;
};

#endif  //MYPLUGIN_H
