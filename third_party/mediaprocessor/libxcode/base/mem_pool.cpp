/* <COPYRIGHT_TAG> */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mem_pool.h"

MemPool::MemPool(): cache_buf_(NULL), rd_ptr_(NULL), wr_ptr_(NULL),
    cache_buf_size_(0), mem_map_size_(0), padding_buf_size_(0),
    data_eof_(false), done_init_(false)
{
    input_name_ = "<no name>";
}

MemPool::MemPool(const char* name): cache_buf_(NULL), rd_ptr_(NULL), wr_ptr_(NULL),
    cache_buf_size_(0), mem_map_size_(0), padding_buf_size_(0),
    data_eof_(false), done_init_(false)
{
    input_name_ = name;
}

MemPool::~MemPool()
{
    if (NULL != cache_buf_) {
        delete [] cache_buf_;
        cache_buf_ = NULL;
    }
}

int MemPool::init(unsigned int buf_size, unsigned int padding_size)
{
    if (done_init_) {
        return MEM_POOL_RETVAL_SUCCESS;
    }

    if (buf_size == 0 || padding_size == 0) {
        // Too small, need to alloc bigger data
        return MEM_POOL_RETVAL_INVALID_INPUT;
    }

    cache_buf_ = new unsigned char[buf_size + padding_size];

    if (NULL == cache_buf_) {
        return MEM_POOL_RETVAL_MEM_ALLOC_FAIL;
    }

    cache_buf_size_ = buf_size;
    padding_buf_size_ = padding_size;
    rd_ptr_ = cache_buf_;
    wr_ptr_ = cache_buf_;
    done_init_ = true;
    return MEM_POOL_RETVAL_SUCCESS;
}

unsigned char *MemPool::GetWritePtr()
{
    return wr_ptr_;
}

unsigned char *MemPool::GetReadPtr()
{
    return rd_ptr_;
}

unsigned int MemPool::GetBufFullness()
{
    if (NULL == cache_buf_ || NULL == wr_ptr_ || NULL == rd_ptr_) {
        return MEM_POOL_RETVAL_SUCCESS;
    }

    // printf("wr_ptr offset %d, rd_ptr offset %d\n", wr_ptr_ - cache_buf_, rd_ptr_ - cache_buf_);
    if (wr_ptr_ >= rd_ptr_) {
        return (wr_ptr_ - rd_ptr_);
    } else {
        return (wr_ptr_ + cache_buf_size_ - rd_ptr_);
    }
}

unsigned int MemPool::GetFreeBufSize()
{
    return cache_buf_size_ - GetBufFullness() - 1;
}

unsigned int MemPool::GetFreeFlatBufSize()
{
    unsigned int free_circulate_buf_size = GetFreeBufSize();
    unsigned int free_flat_buf_size = free_circulate_buf_size;

    if (wr_ptr_ > rd_ptr_) {
        unsigned int free_flat_buf =
            cache_buf_ + cache_buf_size_ + padding_buf_size_ - wr_ptr_;
        free_flat_buf_size = (free_circulate_buf_size <= free_flat_buf ? free_circulate_buf_size : free_flat_buf);
    }

    //printf("Got Free Flat Buf Size %d\n", free_flat_buf_size);
    return free_flat_buf_size;
}

unsigned int MemPool::GetFlatBufFullness()
{
    unsigned int circular_buf_fullness = GetBufFullness();
    unsigned int flat_buf_fullness = circular_buf_fullness;

    if (wr_ptr_ < rd_ptr_) {
        unsigned int wrap_size = wr_ptr_ - cache_buf_;
        MapWrappedBuffer();

        if (wrap_size > padding_buf_size_) {
            flat_buf_fullness = circular_buf_fullness  - wrap_size + padding_buf_size_;
        }
    }

    //printf("Got flatBufFullness %d\n", flat_buf_fullness);
    return flat_buf_fullness;
}

unsigned int MemPool::GetTotalBufSize()
{
    return cache_buf_size_;
}

int MemPool::UpdateReadPtr(unsigned int advance_bytes)
{
    if (!wr_ptr_ || !rd_ptr_ || !cache_buf_) {
        return MEM_POOL_RETVAL_INVALID_POINTER;
    }

    static int total_bytes = 0;
    total_bytes += advance_bytes;

    // printf("-------------Update ReadPtr 0x%x(%d)\n", total_bytes, advance_bytes);
    if (advance_bytes <= GetBufFullness()) {
        //printf("Update read ptr from %d to ", rd_ptr_ - cache_buf_);
        if (rd_ptr_ + advance_bytes <= cache_buf_ + cache_buf_size_) {
            rd_ptr_ += advance_bytes;
        } else {
            // Wrap.
            rd_ptr_ -= cache_buf_size_ - advance_bytes;
            // Invalidate the padding data.
            mem_map_size_ = 0;
        }

        //printf("%d\n", rd_ptr_ - cache_buf_);
        return MEM_POOL_RETVAL_SUCCESS;
    }

    printf("[%p]:Over step happened, adv bytes %d, total size %d\n", 
            this, advance_bytes, GetBufFullness());
    assert(0);
    return MEM_POOL_RETVAL_OVERSTEP;
}

int MemPool::UpdateWritePtr(unsigned int advance_bytes)
{
    if (!wr_ptr_ || !rd_ptr_ || !cache_buf_) {
        return MEM_POOL_RETVAL_INVALID_POINTER;
    }

    if (advance_bytes <= GetFreeBufSize()) {
        if (wr_ptr_ + advance_bytes <= cache_buf_ + cache_buf_size_) {
            wr_ptr_ += advance_bytes;
        } else {
            wr_ptr_ -= cache_buf_size_ - advance_bytes;
        }

        return MEM_POOL_RETVAL_SUCCESS;
    }

    return MEM_POOL_RETVAL_OVERSTEP;
}

int MemPool::UpdateWritePtrCopyData(unsigned int advance_bytes)
{
    if (!wr_ptr_ || !rd_ptr_ || !cache_buf_) {
        return MEM_POOL_RETVAL_INVALID_POINTER;
    }

    // printf("wr_ptr_ offset is %d, advance_byte is %d\n", wr_ptr_ - cache_buf_, advance_bytes);
    if (advance_bytes <= GetFreeFlatBufSize()) {
        if (wr_ptr_ + advance_bytes <= cache_buf_ + cache_buf_size_) {
            wr_ptr_ += advance_bytes;
        } else {
            unsigned int wrap_size = advance_bytes - (cache_buf_ + cache_buf_size_ - wr_ptr_);

            // Copy data first, then change pointer!
            if (wrap_size > 0) {
                memcpy(cache_buf_, cache_buf_ + cache_buf_size_, wrap_size);
            }

            wr_ptr_ = cache_buf_ + wrap_size;
        }

        return MEM_POOL_RETVAL_SUCCESS;
    }

    return MEM_POOL_RETVAL_OVERSTEP;
}

int MemPool::MapWrappedBuffer()
{
    if (!wr_ptr_ || !rd_ptr_ || !cache_buf_) {
        return MEM_POOL_RETVAL_INVALID_POINTER;
    }

    //printf("Doing Mapping wrapped buffer\n");
    if (wr_ptr_ < rd_ptr_) {
        unsigned int wrap_size = wr_ptr_ - cache_buf_;

        // Usually wrap_size <= padding_buf_size_
        if (wrap_size <= padding_buf_size_) {
            if (wrap_size > 0 && mem_map_size_ < wrap_size) {
                // Just copy the increased part of wrapped data
                // printf("copy wrapped data %d to the padding, total readable data is %d \n",
                //         wrap_size - mem_map_size_, wrap_size + cache_buf_ + cache_buf_size_ - rd_ptr_);
                memcpy(cache_buf_ + cache_buf_size_ + mem_map_size_,
                       cache_buf_ + mem_map_size_, wrap_size - mem_map_size_);
                mem_map_size_ = wrap_size;
            } else {
                // Either wrap_size is 0, or we already did the memcpy.
            }
        } else {
            // TODO:
            // May never find a frame.
            // padding_buf_size_ shall be able to contain at least one frame
            if (mem_map_size_ < padding_buf_size_) {
                // printf("copy wrapped data %d to the padding\n", padding_buf_size_ - mem_map_size_);
                memcpy(cache_buf_ + cache_buf_size_ + mem_map_size_,
                       cache_buf_ + mem_map_size_,
                       padding_buf_size_ - mem_map_size_);
                mem_map_size_ = padding_buf_size_;
            }
        }
    }

    return MEM_POOL_RETVAL_SUCCESS;
}

unsigned int MemPool::GetWritePtrOffset()
{
    if (!wr_ptr_ || !rd_ptr_ || !cache_buf_) {
        return MEM_POOL_RETVAL_INVALID_POINTER;
    }

    return (wr_ptr_ - cache_buf_);
}

unsigned int MemPool::GetReadPtrOffset()
{
    if (!wr_ptr_ || !rd_ptr_ || !cache_buf_) {
        return MEM_POOL_RETVAL_INVALID_POINTER;
    }

    return (rd_ptr_ - cache_buf_);
}
