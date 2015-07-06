/* <COPYRIGHT_TAG> */

#ifndef STREAM_H_
#define STREAM_H_

#include <list>
#include <stdio.h>
#include <string>

#include "mutex.h"

using namespace std;

enum Attributes {
    FILE_HEAD = 0,
    FILE_TAIL,
    FILE_CUR
};
/**
 *\brief Transparent Stream class.
 *\details Provides a transparent interface for codec objects,
 * hiding the data channel type.
 */
class Stream
{
public:
    Stream();
    virtual ~Stream();

    /**
     *\brief Opens a file.
     *\param filename File name to be open.
     *\param openWrite true if file is write-only, false for read-only.
     *\returns true on successful opening.
     */
    bool Open(const char *filename, bool openWrite = false);

    /**
     *\brief Opens a memory based stream.
     *\param openWrite true if file is write-only, false for read-only.
     *\returns true on successful opening.
     */
    bool Open();

    /**
     *\brief Opens a memory based stream.
     *\param openWrite true if file is write-only, false for read-only.
     *\returns true on successful opening.
     */
    bool Open(string name);

    /**
     *\brief Reads a block of data from the input stream.
     *\param buffer Preallocated buffer to hold the data.
     *\param size Size in bytes.
     *\param waitData blocks if not enough data is available until either a
     *\packet of data is available or until a time out occurs. Setting this flag
     *\does not guarantee that the data size read will be the data size requested.
     *\returns Byte count actually read.
     */
    size_t ReadBlock(void *buffer, size_t size, bool waitData = true);

    /**
     *\brief Writes a block of data to the output stream.
     *\param buffer Preallocated buffer that holds the data.
     *\param size Size in bytes.
     *\param copy For memory streams only: If true the data will be copied and
     *the input buffer will not be modified; if false then the buffer will be used as it is,
     *and free() will be called when no longer needed, so it must be
     *allocated with malloc() rather than new().
     *\returns Byte count actually wrote.
     */
    size_t WriteBlock(void *buffer, size_t size, bool copy = true);

    /**
     *\brief returns total number of memory blocks.
     *\param dataSize optional pointer to a variable to hold total available data,
     *as calling GetDataSize() before or after can give false results.
     */
    size_t GetBlockCount(size_t *dataSize = NULL);


    /**
     *\brief Reads a number of blocks of data from the input stream.
     *\param buffer Preallocated buffer to hold the data.
     *\param count Number of memory blocks to be read.
     *\returns Byte count actually read.
     */
    size_t ReadBlocks(void *buffer, int count);

    /**
     *\brief Sets the stream attributes(based on file).
     */
    void SetStreamAttribute(int flag);
    /**
     *\brief Sets the end of stream flag.
     */
    void SetEndOfStream(bool endFlag);
    /**
     *\brief Sets the end of stream flag (for memory streams only).
     */
    void SetEndOfStream();
    /**
     *\brief Gets the end of stream flag (for memory streams only).
     */
    bool GetEndOfStream();
    /**
     *\brief Sets the loop tiems.
     */
    void SetLoopTimesOfStream(int loop);
    /**
     *\brief Gets the loop times.
     */
    int GetLoopTimesOfStream();
    /**
     *\brief returns stream name.
     */
    string &GetFilename() {
        return mFileName;
    };

    /**
     *\brief returns total available data (for memory streams only).
     */
    size_t GetDataSize() {
        return mTotalDataSize;
    };

    bool mSafeData;          /**< \brief safe data to the next module*/
protected:
    string &GetInFilename() {
        return mInFileName;
    };

    /**\brief Memory buffer list element datatype*/
    typedef struct {
        void    *buffer;    /**<\brief buffer address.*/
        char    *indx;      /**<\brief current address.*/
        size_t  size;       /**<\brief current data size.*/
    } STREAM_BUFF;

    enum STREAM_TYPE {
        STREAM_TYPE_UNKNOWN,
        STREAM_TYPE_FILE,
        STREAM_TYPE_MEMORY
    };


    /**\brief FILE object for file based streams*/
    FILE               *mFile;
    string              mFileName;
    string              mInFileName;

    /**\brief buffer list for memory based streams*/
    list<STREAM_BUFF>   mBufferList;
    /**\brief total available data size (in bytes)*/
    size_t              mTotalDataSize;

    int                 mTimeout;           /**< \brief Default time out*/
    bool                mEndOfStream;
    int                 mLoopTimes;
    /**\brief Type of stream*/
    STREAM_TYPE     mStreamType;
    Mutex           mutex;
};

#endif /* STREAM_H_ */
