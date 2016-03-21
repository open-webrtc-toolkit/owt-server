/* <COPYRIGHT_TAG> */

#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H

#include <stdlib.h>
#include <map>
#include <iostream>
#include <string.h>
#include <pthread.h>

class LogInstance
{
public:
//#define NONE "\033[m"
//#define MAXLOGFILESIZE 2*1024*1024

    typedef enum{
        FATAL=2,
        ERROR=4,
        WARNING=8,
        INFO=16,
        TRACE=24,
        DEBUG=32
    }LOGLEVEL;

    static int mLogFileCount;
    LogInstance(){};
    LogInstance(const char* moduleName,
                int level,
                bool hasTime,
                bool hasFileName,
                bool hasModuleName,
                bool hasLogFile,
                const char* logFilePath,
                pthread_mutex_t* mutex);
    ~LogInstance();


    //char* GetModuleName();
    bool isErrorEnabled() { return ERROR <= mLevel ? true:false; };
    bool isFatalEnabled() { return FATAL <= mLevel ? true:false; };
    bool isInfoEnabled() { return INFO <= mLevel ? true:false; };
    bool isWarningEnabled() { return WARNING <= mLevel ? true:false; };
    bool isDebugEnabled() { return DEBUG <= mLevel ? true:false; };
    bool isTraceEnabled() { return TRACE <= mLevel ? true:false; };
    bool hasFileName() { return mHasFileName;}

    //bool LogInfo(const char *strFile,const int &iLineNum,const int &iPriority,const char *strMsg,const char *strCatName);
    void Init(void);
    bool LogFunction(const char *strFun, const int &iLineNum, const int &iPriority, const char* color, const char* format, ...);
    bool LogFile(const char* strFile, const char *strFun, const int &iLineNum, const int &iPriority, const char* color, const char* format, ...);
    bool LogPrintf(const char* format, ...);
private:
    bool SaveLogFileForOther(FILE* fp);
    bool WriteLog(const char * szFileName, const char * strMsg);

    int mLevel;
    std::map<int, std::string> mLevelList;
    const char* mLogFilePath;
    const char* mModuleName;
    char mModuleStr[64];
    bool mHasTime;
    bool mHasFileName;
    bool mHasLogFile;
    bool mHasModuleName;
    pthread_mutex_t* mLocalMutex;
};


class LogFactory
{
private:
    LogFactory();
    LogFactory(const LogFactory &);
    LogFactory& operator = (const LogFactory &);
    bool Init();
    void TrimConfigWhitespace(std::string &str);
    void TrimConfigComments(std::string &str);
    void ReadConfigFile(char *fileName);


    std::map<std::string, int> mModulesList;
    std::string mLogFilePath;
    bool mHasTime;
    bool mHasFileName;
    bool mHasLogFile;
    bool mHasModuleName;

public:
    static void SetLogFactoryParams(char* fileName);
    static LogFactory *GetLogFactory();
    static LogFactory *pLogFactory;
    static pthread_mutex_t mMutex;

    LogInstance* CreateLogInstance(const char* moduleName);
};


#endif //DEBUG_INFO_H
