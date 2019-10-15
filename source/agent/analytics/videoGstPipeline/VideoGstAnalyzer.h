//
// Created by xyk on 19-7-9.
//

#ifndef VideoGstAnalyzer_H
#define VideoGstAnalyzer_H

#include <gst/gst.h>
#include <glib-object.h>
#include <gst/pbutils/encoding-profile.h>
#include <string>
#include <boost/thread.hpp>
#include <logger.h>
#include "InternalIn.h"

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

    void emitConnectTo(int connectionID, char* ip, int remotePort);

    int addElementMany();

    void setOutputParam(std::string codec, int width, int height, 
    int framerate, int bitrate, int kfi, std::string algo, std::string pluginName);

    void stopLoop();

    void disconnect(int connectionID);

    static void pad_added_handler(GstElement *src, GstPad *new_pad, GstElement *data);
    static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);
    static void print_one_tag (const GstTagList * list, const gchar * tag, gpointer user_data);

    static void start_feed (GstElement * source, guint size, gpointer data);
    static gboolean push_data (gpointer data);
    static void stop_feed (GstElement * source, gpointer data);

protected:
    static void main_loop_thread(gpointer);
    static GMainLoop *loop;
    static gboolean StreamEventCallBack(GstBus *bus, GstMessage *message, gpointer data);
    void setState();
    void printFPS();
    boost::scoped_ptr<InternalIn> m_internalin;
    guint sourceid;

private:
    GstElement *pipeline, *source, *receive,*detect,*decodebin,*postproc,*h264parse,*videosink,*fakesink,*fpssink,*sendsink, *rtsph264, *videorate;
    
    GstStateChangeReturn ret;

    GThread *m_thread;
    guint m_bus_watch_id;
    GstBus *m_bus;
    boost::thread m_playthread;

    int connectPort;

    //param
    std::string codec;
    std::string algo,pluginName;
    std::string resolution;
    int width,height;
    int framerate,bitrate;
    int kfi; //keyFrameInterval
};

}

#endif //VideoGstAnalyzer_H
