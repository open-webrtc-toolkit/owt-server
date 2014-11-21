/* <COPYRIGHT_TAG> */

/**
 *\file trace_file.h
 *\brief Definition of trace file
 */

#ifndef _TRACE_FILE_H_
#define _TRACE_FILE_H_

#include <fstream>
#include <string>

#define MAX_PATH_NAME_LEN 256

class TraceInfo;

class TraceFile
{
public:
    TraceFile();
    ~TraceFile();

    void add_trace(TraceInfo *);
    static TraceFile *instance(void);
    void init(const char *filename = NULL);

private:
    void add_log(const char *msg);
    void check(void);
    void backup(void);
    void open_file(void);

private:
    bool no_file_;
    std::ofstream file_;
    long size_;

    static TraceFile *instance_;

    static std::string LOG_FILE;

private:
    TraceFile(const TraceFile &);
    TraceFile operator=(const TraceFile &);
};

#endif

