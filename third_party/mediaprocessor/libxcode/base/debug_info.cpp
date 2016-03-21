/* <COPYRIGHT_TAG> */

//#include <json/json.h>
//#include <iostream>
#include <sstream>
#include <stdio.h>
#include <fstream>
#include "debug_info.h"
#include <errno.h>
#include <stdarg.h>
#include <string>
#include <sstream>


#define MAXLINSIZE 2048
#define MAXFILEPATHLENGTH 512

#define LOG_CONFIG_PATH "/tmp/msdk_log_config.ini"
#define NONE "\033[m"
#define MAXLOGFILESIZE 2*1024*1024

LogFactory* LogFactory::pLogFactory = NULL;

pthread_mutex_t LogFactory::mMutex = PTHREAD_MUTEX_INITIALIZER;

LogFactory* LogFactory::GetLogFactory()
{
    if (pLogFactory == NULL) {
        pthread_mutex_lock(&mMutex);
        if (pLogFactory == NULL) {
            pLogFactory = new LogFactory();
            pLogFactory->Init();
        }
        pthread_mutex_unlock(&mMutex);
    }
    return pLogFactory;
}
LogFactory::LogFactory()
{}

// read ini file for config option
void LogFactory::ReadConfigFile(char *fileName)
{
    std::string newSection;
    std::string currentSection;
    /* Open the config file. */
    std::ifstream configFile(fileName);

    if (!configFile.is_open()) {
        exit(1);
    }

    /* Read the config file. */
    while (!configFile.eof()) {
        std::string line;
        getline(configFile, line);
        /* Remove comments and leading and trailing whitespace. */
        TrimConfigComments(line);
        TrimConfigWhitespace(line);

        /* Skip blank lines. */
        if (line.length() == 0) {
            continue;
        }

        /* Match a section header like [stream]. */
        if (line.find('[') == 0 &&
                line.find(']') == line.length() - 1) {
            newSection = line.substr(1, line.length() - 2);

            /* The new section becomes the current section. */
            currentSection = newSection;
        } /* End of match [section]. */

        if (line.find('=') != std::string::npos) {
            std::string key;
            std::string value;
            /* Create a stream string iterator for the line. */
            std::istringstream strStream(line);
            /* Split the line into a key/value pair around "=" */
            getline(strStream, key,   '=');
            getline(strStream, value, '=');
            /* Trim any whitespace from the tokens. */
            TrimConfigWhitespace(key);
            TrimConfigWhitespace(value);

            /* Store the key/value data. */
            if (key.compare("logfile") == 0) {
                mLogFilePath = value;
                mHasLogFile = true;
            } else if (key.compare("hasTime") == 0) {
                mHasTime = atoi(value.c_str());
            } else if (key.compare("hasFilename") == 0) {
                mHasFileName = atoi(value.c_str());
            } else if (key.compare("hasModulename") == 0) {
                mHasModuleName = atoi(value.c_str());
            }

            std::map<std::string, int>::iterator itrl = mModulesList.begin();
            for( ; itrl != mModulesList.end(); itrl++) {
                if (key.compare(itrl->first.c_str()) == 0) {
                    itrl->second = atoi(value.c_str());
                    break;
                }
            }
        } /* End of match key = value. */
    } /* configFile.eof(). */

#if 0
    for(std::map<std::string, int>::iterator itrl = mModulesList.begin(); itrl != mModulesList.end(); itrl++) {
        printf("========modulename = %s(%d)\n", itrl->first.c_str(), itrl->second);
    }
    printf("=======time = %d, filename= %d, logfile=%s\n", mHasTime, mHasFileName, mLogFilePath.c_str());
#endif
    configFile.close();
}


void LogFactory::TrimConfigWhitespace(std::string &str)
{
    std::string delimiters = " \t\r\n";
    str.erase(0, str.find_first_not_of(delimiters)     );
    str.erase(   str.find_last_not_of (delimiters) + 1 );
}

/**
 *  @brief Trim comments starting with # from a string. In-place.
 */
void LogFactory::TrimConfigComments(std::string &str)
{
    size_t pos = str.find('#');

    if (pos != std::string::npos) {
        str.erase(pos);
    }
}

#if 0 
// read json file for config option
bool LogFactory::ReadConfigFile(char* fileName)
{
    Json::Reader reader;
    Json::Value root;

    printf(" --------read json %s\n", fileName);
    std::ifstream is;
    is.open(fileName, std::ios::binary);
    printf(" --------read json 2 \n");
    if (reader.parse(is, root)) {
        printf("============parse json\n");
        std::string code;
        if (root["modules"].isNull())
            return false;

        mModulesList["null"] = root["modules"]["null"].asInt();
        mModulesList["MSDKCodec"] = root["modules"]["MSDKCodec"].asInt();
        mModulesList["VP8DecPlugin"] = root["modules"]["VP8DecPlugin"].asInt();
        mModulesList["MsdkXcoder"] = root["modules"]["MsdkXcoder"].asInt();
        mHasTime = root["time"].asInt();
        mHasFileName = root["filename"].asInt();
        if (root["logfile"].isNull()) {
            mHasLogFile = false;
        }else {
            mHasLogFile = true;
            mLogFilePath = root["logfile"].asString();
        }

        printf("--------null:%d, msdk_code:%d\n", mModulesList["null"], mModulesList["MSDKCodec"]);
        printf("----------log file name = %s\n", mLogFilePath.c_str());
    } else {
        printf("========parse json failed %s\n", strerror(errno));
    }

    is.close();
    return true;
}
#endif

bool LogFactory::Init()
{
    std::fstream file;

    mHasTime = false;
    mHasFileName = false;
    mHasLogFile = false;
    mHasModuleName = false;
    mModulesList["null"] = LogInstance::ERROR;
    mModulesList["MSDKCodec"] = LogInstance::ERROR;
    mModulesList["VP8DecPlugin"] = LogInstance::ERROR;
    mModulesList["MsdkXcoder"] = LogInstance::ERROR;
    mModulesList["OpenCLFilterBase"] = LogInstance::ERROR;
    mModulesList["OpenCLFilterVA"] = LogInstance::ERROR;
    mModulesList["PicDecPlugin"] = LogInstance::ERROR;
    mModulesList["StringDecPlugin"] = LogInstance::ERROR;
    mModulesList["SWDecPlugin"] = LogInstance::ERROR;
    mModulesList["SWEncPlugin"] = LogInstance::ERROR;
    mModulesList["vaapiFrameAllocator"] = LogInstance::ERROR;
    mModulesList["VAplugin"] = LogInstance::ERROR;
    mModulesList["VAXcoder"] = LogInstance::ERROR;
    mModulesList["OpencvVideoAnalyzer"] = LogInstance::ERROR;
    mModulesList["VP8DecPlugin"] = LogInstance::ERROR;
    mModulesList["VP8EncPlugin"] = LogInstance::ERROR;
    mModulesList["YUVWriterPlugin"] = LogInstance::ERROR;
    mModulesList["YUVReaderPlugin"] = LogInstance::ERROR;
    mModulesList["AudioPostProcessing"] = LogInstance::ERROR;
    mModulesList["AudioDecoder"] = LogInstance::ERROR;
    mModulesList["AudioEncoder"] = LogInstance::ERROR;
    mModulesList["AudioPCMReader"] = LogInstance::ERROR;
    mModulesList["AudioPCMWriter"] = LogInstance::ERROR;
    mModulesList["AudioTranscoder"] = LogInstance::ERROR;
    mModulesList["VCSA"] = LogInstance::ERROR;
    mModulesList["SNVA"] = LogInstance::ERROR;
    mModulesList["FEI"] = LogInstance::ERROR;
    mModulesList["AUDIO_FILE"] = LogInstance::ERROR;
    mModulesList["AUDIO_MIXER"] = LogInstance::ERROR;
    mModulesList["VIOA"] = LogInstance::ERROR;
    mModulesList["ASTA"] = LogInstance::ERROR;
    mModulesList["MediaPad"] = LogInstance::ERROR;

    file.open(LOG_CONFIG_PATH, std::ios::in);
    if (file) {
        ReadConfigFile((char*)LOG_CONFIG_PATH);
    }

    return true;
}

LogInstance* LogFactory::CreateLogInstance(const char* moduleName)
{
    LogInstance* pLogInstance = new LogInstance(moduleName,
                                                mModulesList[moduleName],
                                                mHasTime,
                                                mHasFileName,
                                                mHasModuleName,
                                                mHasLogFile,
                                                mLogFilePath.c_str(),
                                                &mMutex);


    return pLogInstance;
}


int LogInstance::mLogFileCount = 0;

LogInstance::LogInstance(const char* moduleName,
                        int level,
                        bool hasTime,
                        bool hasFileName,
                        bool hasModuleName,
                        bool hasLogFile,
                        const char* logFilePath,
                        pthread_mutex_t* mutex)
    :mLevel(level),
     mLogFilePath(logFilePath),
     mModuleName(moduleName),
     mHasTime(hasTime),
     mHasFileName(hasFileName),
     mHasLogFile(hasLogFile),
     mHasModuleName(hasModuleName),
     mLocalMutex(mutex)
{
    Init();

    if (!strcmp(mModuleName, "null"))
        mHasModuleName = false;

    if (mHasModuleName)
        snprintf(mModuleStr, sizeof(mModuleStr), "%s:", mModuleName);

}

#if 0
void LogInstance::SetHasTimeFlag()
{
    mHasTime = true;
}

void LogInstance::SetModuleLevel(int level)
{
    mLevel = level;
}
char* LogInstance::GetModuleName()
{
    return mModuleName;
}
void LogInstance::SetLogFilePath(std::string logFilePath)
{
    if (!logFilePath.empty()) {
        mLogFilePath = logFilePath.c_str();
        mHasLogFile = true;
    } else {

        mHasLogFile = false;
    }
}

void LogInstance::SetHasFileNameFlag()
{
    mHasFileName = true;
}

void LogInstance::SetHasModuleNameFlag()
{
    mHasModuleName = true;

    sprintf(mModuleStr, "%s:", mModuleName);
}
#endif

bool LogInstance::SaveLogFileForOther(FILE* fp)
{
    int fileLen = 0;
    int ret = 0;

    fseek(fp, 0, SEEK_END);
    fileLen = ftell(fp);
    if (fileLen > MAXLOGFILESIZE) {
        std::string path = mLogFilePath;
        int suffixPos = path.find_last_of('.');
        int separatorPos = path.find_last_of('/');
        std::stringstream strCnt;
        //fclose(fp);
        strCnt<< mLogFileCount;
        if (suffixPos && suffixPos > separatorPos) {
            path.insert(suffixPos, strCnt.str().c_str());
        } else {
            path.insert(path.length(), strCnt.str().c_str());
        }
        mLogFileCount++;
        ret = rename(mLogFilePath,  path.c_str());
        return (ret == 0);
    }
    return false;
}
bool LogInstance::WriteLog(const char * szFileName, const char * strMsg)
{

    FILE * fp= NULL;

    pthread_mutex_lock(mLocalMutex);
    fp = fopen(szFileName,"at+");

    if (NULL==fp) {
        pthread_mutex_unlock(mLocalMutex);
        return false;
    }

    if (SaveLogFileForOther(fp)) {
        fclose(fp);
        fp= fopen(szFileName,"at+");

        if (NULL==fp) {
            pthread_mutex_unlock(mLocalMutex);
            return false;
        }
    }
    fprintf(fp,"%s\r\n",strMsg);
    fflush(fp);
    fclose(fp);
    pthread_mutex_unlock(mLocalMutex);
    return true;
}

void LogInstance::Init(void)
{
    mLevelList[FATAL] = "FATAL";
    mLevelList[ERROR] = "ERROR";
    mLevelList[WARNING] = "WARNING";
    mLevelList[INFO] = "INFO";
    mLevelList[DEBUG] = "DEBUG";
    mLevelList[TRACE] = "TRACE";

    memset(mModuleStr, 0, sizeof(mModuleStr));
}
bool LogInstance::LogFile(
                            const char *strFile,
                            const char* strFun,
                            const int &iLineNum,
                            const int &iPriority,
                            const char* color,
                            const char* format, ...)
{
    char logstr[MAXLINSIZE] = {0};
    std::stringstream logMsg;
    va_list argp;
    time_t now;
    struct tm* tnow;
    std::stringstream tstr;

    va_start(argp,format);
    if (NULL==format||0==format[0])
        return false;
    vsnprintf(logstr,MAXLINSIZE-1,format,argp);
    va_end(argp);
    if (mHasTime) {
        time(&now);
        tnow = localtime(&now);
        if (tnow) {
            tstr << (1900+tnow->tm_year) << "/" << (1+tnow->tm_mon) << "/" << tnow->tm_mday << "_" << tnow->tm_hour << ":" << tnow->tm_min << ":" << tnow->tm_sec;
        }
    }
    if (mHasLogFile) {
        if (mHasTime) {
            logMsg << color
                   << "["
                   << tstr.str().c_str()
                   << "]:"
                   << strFile
                   << ":"
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        } else {
            logMsg << color
                   << strFile
                   << ":"
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        }
        WriteLog(mLogFilePath,logMsg.str().c_str());
    } else {
        if (mHasTime)
            printf("%s[%s]:%s:%s%s(%d):%s:%s" NONE,color, tstr.str().c_str(), strFile, mModuleStr, strFun, iLineNum,mLevelList[iPriority].c_str(),logstr);
        else
            printf("%s%s:%s%s(%d):%s:%s" NONE, color, strFile, mModuleStr, strFun, iLineNum,mLevelList[iPriority].c_str(),logstr);
    }
    return true;
}
bool LogInstance::LogFunction(const char *strFun,
                            const int &iLineNum,
                            const int &iPriority,
                            const char* color,
                            const char* format, ...)
{
    char logstr[MAXLINSIZE] = {0};
    std::stringstream logMsg;
    va_list argp;
    time_t now;
    struct tm* tnow;
    std::stringstream tstr;

    va_start(argp,format);
    if (NULL==format||0==format[0])
        return false;

    vsnprintf(logstr,MAXLINSIZE-1,format,argp);
    va_end(argp);
    if (mHasTime) {
        time(&now);
        tnow = localtime(&now);
        if (tnow) {
           tstr << (1900+tnow->tm_year) << "/" << (1+tnow->tm_mon) << "/" << tnow->tm_mday << " " << tnow->tm_hour << ":" << tnow->tm_min << ":" << tnow->tm_sec;
        }
    }
    if (mHasLogFile) {
        if (mHasTime) {
            logMsg << color
                   << "["
                   << tstr.str().c_str()
                   << "]:"
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        } else {
            logMsg << color
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        }
        WriteLog(mLogFilePath,logMsg.str().c_str());
    } else {
        if (mHasTime) {
            printf("%s [%s]:%s%s(%d):%s: %s" NONE, color, tstr.str().c_str(), mModuleStr, strFun,iLineNum, mLevelList[iPriority].c_str(),logstr);
        }else {
            printf("%s %s%s(%d):%s: %s" NONE, color, mModuleStr, strFun,iLineNum, mLevelList[iPriority].c_str(),logstr);
        }
    }
    return true;
}

bool LogInstance::LogPrintf(const char* format, ...)
{
    char logstr[MAXLINSIZE+1];
    va_list argp;
    va_start(argp,format);
    if (NULL==format||0==format[0])
        return false;
    vsnprintf(logstr,MAXLINSIZE,format,argp);
    va_end(argp);
    if (mHasLogFile) {
        WriteLog(mLogFilePath,logstr);
    } else {
        printf("%s", logstr);
    }
    return true;
}

