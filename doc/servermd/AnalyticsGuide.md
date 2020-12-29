##### Table of Contents
* 1.[Introduction](#introduction) 
* 2.[Install dependencies](#dependencies) 
  * 2.1[Install in Docker image](#dependencies1) 
  * 2.2[Install on host machine](#dependencies2) 
  * 2.3[Download models](#dependencies3) 
* 3.[Test Pipelines Shipped with Open WebRTC Toolkit](#test) 
  * 3.1 [Preparation](#test1) 
  * 3.2 [Start MCU](#test2)
  * 3.3 [Test Media Analytics Pipeline](#test3)
* 4.[Develop and Deploy Your Own Media Analytics Pipelines](#develop)
  * 4.1 [Develop Pipelines](#develop1)
  * 4.2 [Deploy Pipelines](#develop2)

## 1.Introduction

In this document we introduce the media analytics functionality provided by [Open WebRTC Toolkit](https://github.com/open-webrtc-toolkit/owt-server), namely OWT, and a step by step guide to implement your own media analytics pipeline with GStreamer and Intel Distribution of OpenVINO.

OWT Media Analytics Architecture

The software block diagram of OWT Media Analytics：
![Analytics Arch](https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/servermd/AnalyticsFlow.jpg)
OWT Server allows client applications to perform media analytics on any stream in the room with Analytics REST API, including the streams from WebRTC access node or SIP node, and streams from video mixing node. 
A successful media analytics request will generate a new stream in the room, which can be subscribed, analyzed, recorded or mixed like other streams.

Process Model

MCU will fork a new process for each analytics request by forming an integrated media analytics pipeline including decoder, pre-processor, video analyzer and encoder through GStreamer pipeline. Compressed bitstreams flow in/out of the media
analytics pipeline through the common Internal In/Out infrastrcture provided by MCU.

In current implementation, for each media analytics worker, only one media analaytics pipeline is allowed to be loaded. The GStreamer pipeline loaded by the analytics worker is controlled by the algorithm ID parameter in analytics
REST request.

## 2.Install dependencies<a name="dependencies"></a>

System Requirement

Hardware： Intel(R) 6th - 8th generation Core(TM) platform, Intel(R) Xeon(R) E3-1200 v4 Family with C226 chipset, Intel(R) Xeon(R) E3-1200 and E3-1500 v5 Family with C236 chipset

OS： CentOS 7.6 or Ubuntu 18.04

### 2.1 Install in Docker* image<a name="dependencies1"></a>

Follow the [gst-analytics.dockerfile](https://github.com/open-webrtc-toolkit/owt-server/blob/master/docker/gst/gst-analytics.dockerfile) to install dependencies that needed to run OWT and analytics agent. 

You can refer to [script](https://github.com/open-webrtc-toolkit/owt-server/blob/master/docker/gst/build_docker_image.sh) to build video analytics running image with OpenVINO and GStreamer installed:
````
cd owt-server/docker
./build_docker_image.sh openvino
````

Then owt:openvino image will be created with all the video analytics needed software installed. Then you can launch the container and copy OWT package into the container:
````
docker run -itd --net=host --name=gst-analytics owt:openvino bash
docker cp Release-vxxx.tgz gst-analytics:/home/
docker exec -it gst-analytics bash
````

### 2.2 Install running dependencies on host machine<a name="dependencies2"></a>

Besides basic OWT dependencies, analytics agent requires OpenVINO and GStreamer to do video analytics. Please download OpenVINO 2021.1.110 and dlstreamer_gst 1.2.1 version and refer to option 3 steps in [dlstreamer_gst install guide](https://github.com/openvinotoolkit/dlstreamer_gst/wiki/Install-Guide#install-on-host-machine) to install OpenVINO, gst-video-analytics plugins and related dependencies. 

For dlstreamer installation in CentOS, required version is >=3.1, if your system installed cmake version is < 3.1, build process will fail. In this case, following steps below:
```
yum remove cmake
wget https://github.com/Kitware/CMake/releases/download/v3.14.5/cmake-3.14.5-Linux-x86_64.tar.gz
tar zxf cmake-3.14.5-Linux-x86_64.tar.gz -C /opt/
export CMAKE_HOME=/opt/cmake-3.14.5-Linux-x86_64
export PATH=$PATH:$CMAKE_HOME/bin
cmake --version
```

Then you can continue to build dlstreamer.

### 2.3 Download models for analytics<a name="dependencies3"></a>

Download [open model zoo package](https://github.com/opencv/open_model_zoo/releases/tag/2020.4) and uncompress file in docker container or host machine:
````
#wget https://github.com/opencv/open_model_zoo/archive/2020.4.tar.gz
#tar zxf 2020.4.tar.gz
#cd open_model_zoo-2020.4/tools/downloader
````
Follow  Model Downloader guide (open_model_zoo-2020.4/tools/downloader/README.md) to install dependencies and then download models. To try OWT analytics sample plugins, you can follow steps below to only download the used model:
````
./downloader.py --name face-detection-retail-0004
````

## 3 Test Pipelines Shipped with Open WebRTC Toolkit<a name="test"></a>

### 3.1 Preparation<a name="test1"></a>

After all the dependencies are installed, you can follow commands below to build sample video analytics plugins in docker container or host machine:
````
tar zxf Release-vxxx.tgz
cd Release-vxxx/analytics_agent/plugins/samples
./build_samples.sh
cp build/intel64/Release/lib/lib* ../../lib/
````

The script builds analytics pipelines, the output dynamic libraries for pipelines are under ````build/intel64/Release/lib/```` directory.

|Pipeline library name        |             Functionality                       |
|---------------------------|-------------------------------------------------|
|libCPUPipeline.so          |Face detection pipeline with CPU pipeline              |
|libDetectPipeline.so       |Face detection with GPU decode and VPU inference                        |
|libSamplePipeline.so       |Dummy pipelines                          |

Copy all output library files to ````analytics_agent/lib/```` directory, or to path specified by ````analytics.libpath````section in ````dist/analytics_agent/agent.toml```` file, 
which is by default ````dist/analytics_agent/pluginlibs/````directory.

In OWT server, the analytics algorithms and pipeline binaries are bound by ````dist/analytics_agent/plugin.cfg````configuration file. ````plugin.cfg````by default contains multiple mappings from algorithm to pipeline binaries, for example:

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

You can specify different model path (downloaded from open model zoo) and other inference parameters to try out different inference models.

Make sure you copy pipeline binaries to ````dist/analytics_agent/lib/````directory or the path specified in ````dist/analytics_agent/agent.toml````.

### 3.2 Start MCU<a name="test2"></a>

Start up MCU in Docker container:

````
cd Release-vxxx
./bin/init-all.sh --deps ##default password for user owt is owt
./bin/start-all.sh
````

Start up OWT in host machine:
````
source /opt/intel/openvino_2021/bin/setupvars.sh
##dl_streamer_lib_path is the build library path in step 2.2
export GST_PLUGIN_PATH=${dl_streamer_lib_path}:/usr/lib/x86_64-linux-gnu/gstreamer-1.0:${GST_PLUGIN_PATH} #for ubuntu18.04
export GST_PLUGIN_PATH=${dl_streamer_lib_path}:/usr/lib64/gstreamer-1.0::${GST_PLUGIN_PATH} #for centos7.6
cd Release-vxxx
cp ${OWT_SOURCE_CODE}/docker/analyticspage/index.js apps/current_app/public/scripts/ 
cp ${OWT_SOURCE_CODE}/docker/analyticspage/rest-sample.js apps/current_app/public/scripts/
cp ${OWT_SOURCE_CODE}/docker/analyticspage/index.html apps/current_app/public/
cp ${OWT_SOURCE_CODE}/docker/analyticspage/samplertcservice.js apps/current_app/public/
./bin/init-all.sh --deps
./bin/start-all.sh
````

### 3.3 Test Media Analytics Pipelines<a name="test3"></a>

Make sure the camera is accessible and start up Chrome browser on your desktop:````https://your_mcu_url:3004````
There might be some security prompts about certificate issue. Select to trust those certificates. You might need to click the link in page: ````Click this for testing certificate and refresh````

Refresh the test page and your local stream should be published to MCU.

#### Start Analytics

On the page, drop down from “video from” and select the remote stream you would like to analyze.


Make sure face detection with cpu pipeline has been installed and model path has been configured correctly in ````analytics_agent/plugin.cfg````. The source code of face detection pipeline is under ````dist/analytics_agent/plugins/samples/cpu_pipeline/````, you can change the model path to try other detection models.


Check ````analytics_agent/plugin.cfg````， The pipeline ID for face detection with CPU is ````dc51138a8284436f873418a21ba8cfa9````, so on the page, in "pipelineID" input text, input the pipeline ID in "analytics id" 
edit control, that is,  ````dc51138a8284436f873418a21ba8cfa9```` without any extra spaces, and press "startAnalytics" button. The stream you selected will be analyzed, with annotation on the faces in the stream.

**Note:** If the sample cpu_pipeline analytics failed, please debug as following:  
a. Check sample cpu_pipeline is successfully compiled and generated libraries are copied to ```analytics_agent/lib``` folder  
b. Check ```modelpath``` in ```analytics_agent/plugin.cfg``` and make sure it is configured to the correct openmodel zoo model path.  
c. For sample cpu_pipeline, the failure is mostly caused by gvadetect and x264enc elements cannot be found, use ```gst-inspect-1.0 gvadetect``` and ```gst-inspect-1.0 x264enc``` to check if used GStreamer elements are successfully installed or configured. If x264enc element is not found, please make sure ```gstreamer1-plugins-ugly``` (for CentOS) and ```gstreamer1.0-plugins-ugly``` (for Ubuntu). If gvadetect element is not found, please make sure dlstreamer is successfully compiled. Check environment variable ```GST_PLUGIN_PATH``` and add x264 and gvadetect libraries to the ```GST_PLUGIN_PATH``` as:  
````
export GST_PLUGIN_PATH=${dl_streamer_lib_path}:/usr/lib/x86_64-linux-gnu/gstreamer-1.0:${GST_PLUGIN_PATH} #for ubuntu18.04
export GST_PLUGIN_PATH=${dl_streamer_lib_path}:/usr/lib64/gstreamer-1.0::${GST_PLUGIN_PATH} #for centos7.6
````

#### Subscribe analyzed stream

On the page and drop down from "subscribe video" and select the stream id with ````algorithmid+video from streamid```` and click subscribe, then analyzed stream will display on page.

**Note that if you do not add GStreamer elements ````x264enc + appsink```` into your pipeline like sample detect_pipeline, analyzed stream will not be sent back to OWT server so you cannot subscribe analyzed stream.**

#### Stop Analytics

After you successfully start analytics, analytics id will be generated and the latest analytics id will display in ```analytics id:``` on page, then click ```stopAnalytics``` button on page to stop analytics. Or you can click ```listAnalytics``` button to list all started analytics id on Chrome console, and input the analytics id in ```analytics id:```, then click ```stopAnalytics``` button on page to stop analytics .


## 4 Develop and Deploy Your Own Media Analytics Pipelines<a name="develop"></a>

MCU supports implementing your own media analytics pipelines and deploy them. Below are the detailed steps. You can also refer to dummy pipeline(in  ````plugins/samples/sample_pipeline/````) or face detection pipeline with CPU(````plugins/samples/cpu_pipeline/````)or face detection pipeline with GPU and VPU(````plugins/samples/detect_pipeline/````) for more details.


### 4.1 Develop Pipelines<a name="develop1"></a>

Pipeline exists in the format of an implementation class of ````rvaPipeline```` interface,  which will be built into an .so library. The defnition of ````rvaPipeline```` interface is under ````plugins/include/pipeline.h````.

````
class rvaPipeline {
 public:
  /// Constructor
  rvaPipeline() {}
  /**
   @brief Initializes a pipeline with provided params after MCU creates the pipeline.
   @param params unordered map that contains name-value pair of parameters
   @return RVA_ERR_OK if no issue initialize it. Other return code if any failure.
  */
  virtual rvaStatus PipelineConfig(std::unordered_map<std::string, std::string> params) = 0;
  /**
   @brief Release internal resources the pipeline holds before MCU destroy the pipeline.
   @return RVA_ERR_OK if no issue close the pipeline. Other return code if any failure.
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
The main interfaces for a pipeline implementation are ```PipelineConfig()```,  ````InitializePipeline()```` and````LinkElements()````.

In ````PipelineConfig()````implemntation，you should get the width, height and frame rate of input stream, and you will also get the algorithm name which is aligned with settings in ```plugin.cfg```, then you can use this algorithm name in ````LinkElements()```` to load pipeline settings such as inferencing model path, inference width and height, as well as configuring the device to decode and inference.

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

The API ````InitializePipeline()```` create elements and pipeline, and its prototype is：
````
GstElement * InitializePipeline();
````
Here are 2 pipelines:
![pipeline examples](https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/servermd/AnalyticsPipeline.jpg)

**Please follow rules below when adding elements to pipeline:**

**1. The first element in pipeline should be ```appsrc``` and its element name should be ```appsource```**

**2. If you want to send analyzed stream back to OWT server, make sure add an encode element(like x264enc) to encode stream, the element name should be ```encoder```**

**3. If you want to send analyzed stream back to OWT server, the last element in pipeline should be ```appsink``` and the element name should be ```appsink```**

You can customize  the pipeline by constructing different GStreamer elements to implement video analytics tasks of different purposes.

Below we show the InitializePipeline implementation in cpu_pipeline pipeline with CPU to decode, inference and encode.

|pipeline library name        |             Functionality                       |
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

The ````LinkElements```` interface set properties for elements in pipeline and link elements in pipeline. To check what properties each element has, please go to GStreamer documents, take ```gvadetect``` for example, you can refer to [gvadetect doc](https://github.com/openvinotoolkit/dlstreamer_gst/wiki/gvadetect), and set gvadetect properties, you can follow:
````
g_object_set(G_OBJECT(detect),"device", device.c_str(),
        "model",model.c_str(),
        "nireq", 24, NULL);
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
    "profile", G_TYPE_STRING, "constrained-baseline", NULL);

link_ok = gst_element_link_filtered (encoder, outsink, encodecaps);
gst_caps_unref (encodecaps);
````

After you finish ````rvaPipeline```` interface implementation，Please include such **DECLARE_PIPELINE** macro to delcare the pipeline to MCU：
````
DECLARE_PIPELINE(YourPipelineName)
````

Here ````YourPipelineName```` must be the class name of your ````rvaPipeline```` implementation.

Finally build and copy all binaries to the same directory where you put pipelines shipped with MCU, including the libaries your pipeline depends on that are not within ````LD_LIBRARY_PATH````.

### 4.2 Deploy Pipelines<a name="develop2"></a>

Like other pipelines shipped with MCU, you need to add an entry into ````dist/analytics_agent/plugin.cfg```` for your pipeline by generating a new UUID, using that to start a new section in ````plugin.cfg````.

Restart analytics agent to make the changes effective, and then you can use the new UUID(the pipeline ID) to test your pipeline:
````
bin/daemon.sh stop analytics-agent 
bin/daemon.sh start analytics-agent 
````

You can refer to GStreamer documents for check elements of different usage to construct your own video analytics tasks.
