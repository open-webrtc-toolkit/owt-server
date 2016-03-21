/* <COPYRIGHT_TAG> */

#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stdlib.h>
#include <string>

#define MEM_POOL_RETVAL_BASE            (0)
#define MEM_POOL_RETVAL_SUCCESS         MEM_POOL_RETVAL_BASE + 0
#define MEM_POOL_RETVAL_INVALID_POINTER MEM_POOL_RETVAL_BASE + 1
#define MEM_POOL_RETVAL_OVERSTEP        MEM_POOL_RETVAL_BASE + 2
#define MEM_POOL_RETVAL_MEM_ALLOC_FAIL  MEM_POOL_RETVAL_BASE + 3
#define MEM_POOL_RETVAL_INVALID_INPUT   MEM_POOL_RETVAL_BASE + 4

/*! \class MemPool
 *  \brief A memory pool class.
 *
 *  Implemented with Ring Buffer+padding buffer.
 *  User can get read/write pointer and use them directly.
 *  While writting, if wrap happened, it will write to the padding buffer first.
 *  The wrapped data can be copied to the beginning of Ring Buffer manually.
 *  While reading, it can first map the wrapped data into padding buffer(flat mode).
 *  Then the parser/decoder can get a continuous memory of data.
 *
*/
class MemPool
{
private:
    unsigned char *cache_buf_;
    unsigned char *rd_ptr_;
    unsigned char *wr_ptr_;
    unsigned int cache_buf_size_;
    unsigned int mem_map_size_;
    unsigned int padding_buf_size_;
    bool data_eof_;
    bool done_init_;
    const char* input_name_;

    MemPool(const MemPool&);
    MemPool& operator=(const MemPool&);
public:
    MemPool();
    MemPool(const char* name);
    ~MemPool();

    /*! \fn int init(unsigned int buf_size, unsigned int padding_size)
     *
     *  \brief The init function for the memory pool.
     *  \param buf_size The memory pool buffer size.
     *  \param padding_size The padding buffer size.
     *  \return MEM_POOL_RETVAL_SUCCESS: Success.
     *          MEM_POOL_RETVAL_INVALID_INPUT: Input parameter invalid.
     *          MEM_POOL_RETVAL_MEM_ALLOC_FAIL: Not enough memory.
     */
    int init(unsigned int buf_size = 4 * 1024 * 1024,
             unsigned int padding_size = 1024 * 1024);

    void SetDataEof(bool eof = true) {
        data_eof_ = eof;
    }

    bool GetDataEof() {
        return data_eof_;
    }

    /*! \fn unsigned char* GetWritePtr()
     *  \brief Get the write pointer.
     *  \return The write pointer.
     */
    unsigned char *GetWritePtr();

    unsigned int GetWritePtrOffset();
    unsigned int GetReadPtrOffset();
    /*! \fn unsigned char* GetReadPtr()
     *  \brief Get the read pointer.
     *  \return The read pointer.
     */
    unsigned char *GetReadPtr();

    /*! \fn unsigned int GetBufFullness()
     *  \brief Get the data size in the mem pool.
     *  \return Return the data amount in the buf.
     */
    unsigned int GetBufFullness();

    /*! \fn unsigned int GetFlatBufFullness()
     *  \brief Get the continuous data size(flat mode).
     *         The wrapped buffer will be copied into the padding buffer for use.
     */
    unsigned int GetFlatBufFullness();

    /*! \fn unsigned int GetFreeBufSize()
     *  \brief Get the free buffer size in the mem pool.
     */
    unsigned int GetFreeBufSize();

    /*! \fn unsigned int GetFreeFlatBufSize()
     *  \brief Get the free buffer size in the mem pool(flat mode).
     */
    unsigned int GetFreeFlatBufSize();

    /*! \fn unsigned int GetTotalBufSize()
     *  \brief Get the buffer size in the mem pool.
     */
    unsigned int GetTotalBufSize();

    /*! \fn int MapWrappedBuffer()
     *  \brief Copy the wrapped buffer into padding buffer.
     *  \return MEM_POOL_RETVAL_SUCCESS: Succeed
     *          MEM_POOL_RETVAL_INVALID_POINTER: The MemPool is not initialized properly.
     */
    int MapWrappedBuffer();

    /*! \fn int UpdateReadPtr(unsigned int advance_bytes)
     *  \brief Update the read pointer. Advance the read pointerby advance_bytes.
     *  \param advance_bytes Advance the read ptr by advance_bytes.
     *  \return MEM_POOL_RETVAL_SUCCESS: Succeed
     *          MEM_POOL_RETVAL_INVALID_POINTER: The MemPool is not initialized properly.
     */
    int UpdateReadPtr(unsigned int advance_bytes);

    /*! \fn int UpdateWritePtr(unsigned int advance_bytes)
     *  \brief Update the write pointer. Advance the write pointer by advance_bytes.
     *  \param advance_bytes Advance the write pointer by advance_bytes.
     *  \return MEM_POOL_RETVAL_SUCCESS: Succeed
     *          MEM_POOL_RETVAL_INVALID_POINTER: The MemPool is not initialized properly.
     */
    int UpdateWritePtr(unsigned int advance_bytes);

    /*! \fn int UpdateWritePtrCopyData(unsigned int advance_bytes)
     *  \brief Update the write pointer. Advance the write pointer by advance_bytes.
     *         If wrap happens, also copy the data in padding buffer into "main" buffer(wrap).
     *  \param advance_bytes Advance the write pointer by advance_bytes.
     *  \return MEM_POOL_RETVAL_SUCCESS: Succeed
     *          MEM_POOL_RETVAL_INVALID_POINTER: The MemPool is not initialized properly.
     *          MEM_POOL_RETVAL_OVERSTEP: Over the boundary.
     */
    int UpdateWritePtrCopyData(unsigned int advance_bytes);

    const char* GetInputName() {
        return input_name_;
    }
    void SetInputName(const char* name)
    {
        input_name_ = name;
    }
};

#endif
