Using Open WebRTC Toolkit For Media Analytics
=============================================
In this document we introduce the media analytics functionality provided by Open WebRTC Toolkit(https://github.com/open-webrtc-toolkit/owt-server), namely OWT, and a step by step guide to implement your own 
media analytics plugins with GStreamer pipeline and Intel Distribution of OpenVION.

OWT Media Analytics Architecture
================================

The software block diagram of OWT Media Analytics：
![Analytics Arch](https://github.com/open-webrtc-toolkit/owt-server/blob/gst-analytics/doc/servermd/AnalyticsFlow.jpg)
OWT Server allows client applications to perform media analytics on any stream in the room with Analytics REST API, including the streams from WebRTC access node or SIP node, and streams from video mixing node. 
A successful media analytics request will generate a new stream in the room, which can be subscribed, analyzed, recorded or mixed like other streams.


Process Model
-------------
MCU will fork a new process for each analytics request by forming an integrated media analytics pipeline including decoder, pre-processor, video analyzer and encoder. Compressed bitstreams flow in/out of the media
analytics pipeline through the common Internal In/Out infrastrcture provided by MCU.

In current implementation, for each media analytics pipeline, only one media analaytics plugin is allowed to be loaded. The plugin loaded by the pipeline is controlled by the algorithm ID parameter in analytics
REST request.

Build and Installation
======================

System Requirement
------------------

Hardware： Intel(R) 6th - 8th generation Core(TM) platform with Intel integrated graphics.

OS： CentOS 7.4/7.6 or Ubuntu 16.04/18.04

Installation Steps
------------------

### Build Docker image

Follow the Dockerfile from <https://github.com/open-webrtc-toolkit/owt-server/blob/gst-analytics/docker/gst/Dockerfile> to check dependencies for compiling OWT , running OWT and running analytics environment. 

You can refer to <https://github.com/open-webrtc-toolkit/owt-server/blob/gst-analytics/docker/gst/build_docker_image.sh> to build images you need:

- OWT compile docker environment
````
docker build --target owt-build -t gst-owt:build \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .
````

- OWT running environment without analytics 
````
docker build --target owt-run -t gst-owt:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .
````

- Only analytics agent running environment
````
docker build --target analytics-run -t gst-analytics:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .
````

- OWT running environment including analytics
````
docker build --target owt-run-all -t gst-owt-all:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .
````

### Download models for analytics
Download open model zoo package from <https://github.com/opencv/open_model_zoo/releases/tag/2019_R3.1> and uncompress file:
````
#wget https://github.com/opencv/open_model_zoo/archive/2019_R3.1.tar.gz
#tar zxf 2019_R3.1.tar.gz
#cd open_model_zoo-2019_R3.1/tools/downloader
````
Then follow  Model Downloader<tools/downloader/README.md> to install open model zoo downloading dependencies and download models.


### Build analytics plugin

After Docker image has been successfully created, run following steps to launch container, take owt running including analytics environment as an example, assume that models downloaded from open model zoo are placed in models folder:
````
#docker run -u root -v ~:/mnt -v /var/tmp:/var/tmp --privileged --net=host $(env | grep -E '_(proxy)=' | sed 's/^/-e /') --entrypoint bash -tid gst-owt-all:run

#docker ps
CONTAINER ID        IMAGE               COMMAND             CREATED             STATUS              PORTS               NAMES
a1021ae56508        gst-owt-all:run   "bash"              29 seconds ago      Up 27 seconds                           silly_sammet

#docker exec -it a1021ae56508 bash
/home# ls /mnt/models/
face-detection-retail-0004.bin  face-detection-retail-0004.xml
/home# cd /home/owt/analytics_agent/plugins/samples
/home/owt/analytics_agent/plugins/samples# ./build_samples.sh
````
The script build analytics plugins, the output dynamic libraries for plugins are under ````build/intel64/Release/lib/```` directory.

|Plugin library name        |             Functionality                       |
|---------------------------|-------------------------------------------------|
|libCPUPipeline.so          |Face detection plugin with CPU pipeline              |
|libDetectPipeline.so       |Face detection with GPU decode and VPU inference                        |
|libSamplePipeline.so       |Dummy plugins that shows how to write plugins                          |

Copy all output library files to ````analytics_agent/lib/```` directory, or to path specified by ````analytics.libpath````section in ````dist/analytics_agent/agent.toml```` file, 
which is by default ````dist/analytics_agent/libs/````directory.


Test Plugins Shipped with Open WebRTC Toolkit
=============================================
We introduce the usage of plugins shipped with OWT here, using face detection with CPU as examples:

Preparation
-----------

In OWT server, the analytics algorithms and plugin binaries are bound by ````dist/analytics_agent/plugin.cfg````configuration file.

````plugin.cfg````by default contains multiple mappings from algorithm to plugin binaries, for example:
````
[dc51138a8284436f873418a21ba8cfa9]
description = 'detect plugin'
pluginversion = 1
apiversion = 400
name = 'libCPUPipeline.so'
libpath = 'pluginlibs/'
configpath = 'pluginlibs/'
modelpath = '/mnt/models/face-detection-retail-0004.xml'    # inference model path
inferencewidth = 672    # inference input width.
inferenceheight = 384   # inference input height.
inferenceframerate = 5  # inference input framerate
device = "CPU"
````

Make sure you copy plugin binaries to ````dist/analytics_agent/lib/````directory or the path specified in ````dist/analytics_agent/agent.toml````.


Configure and Start MCU
-----------------------

Go to dist directory of MCU and finish the initialization：

````bin/init-all.sh````


Start up MCU(make sure you set OpenVION enviroment before starting MCU):

````bin/start-all.sh````


Test Media Analytics Plugins
----------------------------

Make sure the camera is accessible and start up Chrome browser on your desktop:

````https://your_mcu_url:3004````

There might be some security prompts about certificate issue. Select to trust those certificates. You might need to click the link in page: ````Click this for testing certificate and refresh````

Refresh the test page and your local stream should be published to MCU.


### Start Analytics

On the page, drop down from “video from” and select the remote stream you would like to analyze.


Make sure face detection with cpu plugin has been installed. The source code of face detection plugin is under ````dist/analytics_agent/plugins/samples/cpu_pipeline/````, you can modify the implementatio if neccessary.


Check ````analytics_agent/plugin.cfg````， The plugin ID for face detection with CPU is ````dc51138a8284436f873418a21ba8cfa9````, so on the page, in "pipelineID" input text, input the plugin ID in "analytics id" 
edit control, that is,  ````dc51138a8284436f873418a21ba8cfa9```` without any extra spaces, and press "startAnalytics" button. The stream you selected will be analyzed, with annotation on the faces in the stream.

### Subscribe analyzed stream

On the page and drop down from "subscribe video" and select the stream id with ````algorithmid+video from streamid```` and click subscribe, then analyzed stream will display on page.

Note that if you do not add GStreamer plugin ````x264enc + appsink```` in your pipeline like sample detect_pipeline, analyzed stream will not be send back to OWT server so you cannot subscribe analyzed stream.  

### Stop Analytics

After you successfully start analytics, analytics id will be generated and the latest analytics id will display in ```analytics id:``` on page, then click ```stopAnalytics``` button on page to stop analytics. Or you can click ```listAnalytics``` button to list all started analytics id, and input the analytics id in ```analytics id:```, then click ```stopAnalytics``` button on page to stop analytics .

Note that if you do not add GStreamer plugin ````x264enc + appsink```` in your pipeline like sample detect_pipeline, analyzed stream will not be send back to OWT server so you cannot subscribe analyzed stream.


Develop and Deploy Your Own Media Analytics Plugins
===================================================

MCU supports implementing your own media analytics plugins and deploy them. Below are the detailed steps. You can also refer to dummy plugin(in  ````plugins/samples/sample_pipeline/````) or face detection plugin with CPU(````plugins/samples/cpu_pipeline/````)or face detection plugin with GPU and VPU(````plugins/samples/detect_pipeline/````) for more details.


Develop Plugins
---------------

Plugin exists in the format of an implementation class of ````rvaPlugin```` interface,  which will be built into an .so library. The defnition of ````rvaPlugin```` interface is under ````plugins/include/pipeline.h````.

````
class rvaPipeline {
 public:
  /// Constructor
  rvaPipeline() {}
  /**
   @brief Initializes a plugin with provided params after MCU creates the plugin.
   @param params unordered map that contains name-value pair of parameters
   @return RVA_ERR_OK if no issue initialize it. Other return code if any failure.
  */
  virtual rvaStatus PipelineConfig(std::unordered_map<std::string, std::string> params) = 0;
  /**
   @brief Release internal resources the plugin holds before MCU destroy the plugin.
   @return RVA_ERR_OK if no issue close the plugin. Other return code if any failure.
  */
  virtual rvaStatus PipelineClose() = 0;
  /**
   @brief MCU will use this interface to fetch current applied params in the pipeline.
   @param params name-value pair will be returned to the MCU provided unordered_map.
   @return RVA_ERR_OK if params are successfull filled in, or empty param is provided. 
           Other return code if any failure.
  */
  virtual rvaStatus GetPipelineParams(std::unordered_map<std::string, std::string> &params) = 0; 
  /**
   @brief MCU will use this interface to update params in the pipeline.
   @param params name-value pair to be set.
   @return RVA_ERR_OK if params are successfull updated. 
           Other return code if any failure.
  */
  virtual rvaStatus SetPipelineParams(std::unordered_map<std::string, std::string> params) = 0;

  /**
   @brief MCU will use this interface to initiate elements in pipeline.
   @return a new pipeline if elements and pipeline are successfully created. 
           Other return NULL if any failure.
  */
  virtual GstElement * InitializePipeline() = 0;

  /**
   @brief MCU will use this interface to link elements in the pipeline.
   @return RVA_ERR_OK if pipeline is successfull linked. 
           Other return code if any failure.
  */
  virtual rvaStatus LinkElements() = 0;
};                                                                                  
````                                                                                                

The main interfaces for a plugin implementation are ```PipelineConfig()```,  ````InitializePipeline()```` and````LinkElements()````.

In ````PipelineConfig()````implemntation，you should get the width, height and frame rate of input stream, and you will also get the algorithm name which is aligned with settings in ```plugin.cfg```, and you can use this algorithm name in ````LinkElements()```` to load pipeline settings such as inferencing model path, inference width and height, as well as configuring the device to decode and inference.

The sample code of getting algorithm id and input stream info:
````
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
        std::cout << "pipeline name is not found" << std::endl;
    else
        pipelinename = name->second;
````	

By default we will set 3 environment variables in bin/daemon.sh,  
````
cd ${OWT_HOME}/analytics_agent
export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
export PATH=./bin:/opt/intel/mediasdk/bin:${PATH}
export CONFIGFILE_PATH=./plugin.cfg
```` 
Then we use toml parser to load parameters configured for each algorithm defined in ```plugin.cfg```, here is an example on how to pase toml file ```plugin.cfg``` and get defined parameters such as model path, inferencewidth, inferenceheight .etc.
````
const char* path = std::getenv("CONFIGFILE_PATH");
const auto data = toml::parse(path);
const auto& pipelineconfig = toml::find(data, pipelinename.c_str());
const auto model = toml::find<std::string>(pipelineconfig, "modelpath");
const auto inferencewidth = toml::find<std::int64_t>(pipelineconfig, "inferencewidth");
const auto inferenceheight = toml::find<std::int64_t>(pipelineconfig, "inferenceheight");
const auto inferenceframerate = toml::find<std::int64_t>(pipelineconfig, "inferenceframerate");
const auto device = toml::find<std::string>(pipelineconfig, "device");
````

````InitializePipeline()```` create elements and pipeline, and its prototype is：
````
GstElement * InitializePipeline();
````

**Please follow rules below when adding elements to pipeline:**

**1. The first element in pipeline should be ```appsrc``` and its element name should be ```appsource```**

**2. If you want to send analyzed stream back to OWT server, you need add an encode element(like x264enc) to encode stream, the element name should be ```encoder```**

**3. If you want to send analyzed stream back to OWT server, the last element in pipeline should be ```appsink``` and the element name should be ```appsink```**

You can create GStreamer elements that will be used to process and analyze stream in the pipeline.

Below we show the InitializePipeline implementation in cpu_pipeline plugin with CPU to decode, inference and encode.

|Plugin library name        |             Functionality                       |
|---------------------------|-------------------------------------------------|
|appsrc                     |Used by OWT to insert data into the pipeline     |
|h264parse                  |Parses H.264 streams                             |
|avdec_h264                 |libav h264 decoder                               |
|videoconvert               |Convert video frames between a great variety of video formats|
|gvadetect                  |Run inference to detect object                   |
|gvawatermark               |Draw detection/classification/recognition results on top of video data|
|x264enc                    |Encodes raw video into H264 compressed data      |
|appsink                    |Make the OWT get a handle on the data in the pipeline|

````
 /* Initialize GStreamer */
    gst_init(NULL, NULL);

    /* Create the elements */
    source = gst_element_factory_make("appsrc", "appsource");
    h264parse = gst_element_factory_make("h264parse","parse"); 
    decodebin = gst_element_factory_make("avdec_h264","decode");
    postproc = gst_element_factory_make("videoconvert","postproc");
    detect = gst_element_factory_make("gvadetect","detect"); 
    watermark = gst_element_factory_make("gvawatermark","rate");
    converter = gst_element_factory_make("videoconvert","convert");
    encoder = gst_element_factory_make("x264enc","encoder");
    outsink = gst_element_factory_make("appsink","appsink");

    /* Create the empty VideoGstAnalyzer */
    pipeline = gst_pipeline_new("cpu-pipeline");
````

The ````LinkElements```` interface set properties for elements in pipeline and link elements in pipeline. To check what properties each element has, please go to GStreamer documents, take ```gvadetect``` for example, you can refer to ```gvadetect doc```<https://github.com/opencv/gst-video-analytics/wiki/gvadetect>, and set gvadetect properties, you can follow:
````
g_object_set(G_OBJECT(detect),"device", device.c_str(),
        "model",model.c_str(),
        "cpu-streams", 12,
        "nireq", 24,
        "inference-id", "dtc", NULL);
````
In our example, device and model path are configured in ```plugin.cfg```, so you can use different devices and models to inference by adding different algorithm configuration in ```plugin.cfg```. You can also customize your own parameters in ```plugin.cfg``` and load them.

With necessary properties set for all elements, you can add elements to pipeline and link elements following:
````
gst_bin_add_many(GST_BIN (pipeline), source,decodebin,watermark,postproc,h264parse,detect,converter, encoder,outsink, NULL);

if (gst_element_link_many(source,h264parse,decodebin, postproc, detect, watermark,converter, encoder, NULL) != TRUE) {
        std::cout << "Elements source,decodebin could not be linked." << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }
````

In some cases, you need add gstcaps between 2 elements to negotiate with upstream element to get the exact format you need. Take x264enc as an example, x264enc source pad has lots of capabilities, you can check its sink pad and src pad capabilities with:
````
gst-inspect-1.0 x264enc
````
Then you can see x264 pad capabilities and properties:
````
Pad Templates:
  SINK template: 'sink'
    Availability: Always
    Capabilities:
      video/x-raw
              framerate: [ 0/1, 2147483647/1 ]
                  width: [ 16, 2147483647 ]
                 height: [ 16, 2147483647 ]
                 format: { (string)Y444, (string)Y42B, (string)I420, (string)YV12, (string)NV12, (string)Y444_10LE, (string)I422_10LE, (string)I420_10LE }
  
  SRC template: 'src'
    Availability: Always
    Capabilities:
      video/x-h264
              framerate: [ 0/1, 2147483647/1 ]
                  width: [ 1, 2147483647 ]
                 height: [ 1, 2147483647 ]
          stream-format: { (string)avc, (string)byte-stream }
              alignment: au
                profile: { (string)high-4:4:4, (string)high-4:2:2, (string)high-10, (string)high, (string)main, (string)baseline, (string)constrained-baseline, (string)high-4:4:4-intra, (string)high-4:2:2-intra, (string)high-10-intra }

Element has no clocking capabilities.
Element has no URI handling capabilities.
Element Properties:
````
Here is an example on how to set caps between ```x264enc``` and ```outsink```:
````
gboolean link_ok;
GstCaps* encodecaps = gst_caps_new_simple("video/x-h264",
    "stream-format", G_TYPE_STRING, "byte-stream",
    "width", G_TYPE_INT, inputwidth,
    "height", G_TYPE_INT, inputheight,
                "profile", G_TYPE_STRING, "constrained-baseline", NULL);

link_ok = gst_element_link_filtered (encoder, outsink, encodecaps);
gst_caps_unref (encodecaps);
````

After you finish ````rvaPlugin```` interface implementation，Please include such **DECLARE_PLUGIN** macro to delcare the plugin to MCU：
````
DECLARE_PLUGIN(YourPluginName)
````


Here ````YourPluginName```` must be the class name of your ````rvaPlugin```` implementation.

Finally build and copy all binaries to the same directory where you put plugins shipped with MCU, including the libaries you plugin depends on that are not within ````LD_LIBRARY_PATH````.


Deploy Plugins
--------------

Like other plugins shipped with MCU, you need to add an entry into ````dist/analytics_agent/plugin.cfg```` for your plugin by generating a new UUID, using that to start a new section in ````plugin.cfg````.

Restart analytics agent to make the changes effective, and then you can use the new UUID(the plugin ID) to test your plugin:
````
bin/daemon.sh stop analytics-agent 
bin/daemon.sh start analytics-agent 
````

You are refer to GStreamer documents for more elements to construct different pipelines with different purpose.
