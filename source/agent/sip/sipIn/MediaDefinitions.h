#ifndef MEDIARECEIVER_H
#define MEDIARECEIVER_H

#include <node.h>
#include <node_object_wrap.h>
#include <MediaDefinitions.h>


/*
 * Wrapper class of erizo::MediaSink
 */
class MediaSink : public node::ObjectWrap {
public:
  erizo::MediaSink* msink;
};

#endif
