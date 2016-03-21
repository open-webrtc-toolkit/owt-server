/* <COPYRIGHT_TAG> */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpegts.h"

#define TRANSPORT_PACKET_SIZE (188)
#define TRANSPORT_SYNC_BYTE (0x47)
#define PAT_PID 0

MpegTsDemux::MpegTsDemux(): pmt_pid_(0), pmt_parsed_(false)
{
}

MpegTsDemux::~MpegTsDemux()
{
}



int MpegTsDemux::SetEOF()
{
    // TODO:
    // Get EOF
    std::map<unsigned int, void *>::iterator pid_map_it;

    for (pid_map_it = pid_mempool_map_.begin();
            pid_map_it != pid_mempool_map_.end();
            ++pid_map_it) {
        MemPool *output_mem = static_cast<MemPool *>(pid_map_it->second);

        if (output_mem) {
            printf("Set output mempool %p eof\n", output_mem);
            output_mem->SetDataEof();
        }
    }

    return 0;
}

int MpegTsDemux::DemultiplexStream(unsigned char *buf, unsigned buf_size)
{
    int lastPacketNumber = (buf_size / TRANSPORT_PACKET_SIZE) - 1;
    unsigned char *transportPacket = NULL;
    unsigned int demuxed_data_size = 0;

    /* Ensure that we have at least one full packet. */
    if (buf_size < TRANSPORT_PACKET_SIZE) {
        // Lack data
        return 0;
    }

    /* Iterate through the transport stream packets in the input frame. */
    for (int packetNumber = 0;
            packetNumber <= lastPacketNumber;
            packetNumber++) {

        demuxed_data_size += TRANSPORT_PACKET_SIZE;

        /* Pointer to the next 188 byte transport stream packet. */
        transportPacket = &buf[packetNumber * TRANSPORT_PACKET_SIZE];

        /* Ensure that we have an valid packet. */
        if (transportPacket[0] != TRANSPORT_SYNC_BYTE) {
            // Invalid TS packet
            printf("Don't see SYNC WORD, ignore\n");
            continue;
            //assert(0);
            //return -1;
        }

        /* Figure out how much of this Transport Packet contains PES data. */
        unsigned char adaptation_field_control = (transportPacket[3] & 0x30) >> 4;
        unsigned char totalHeaderSize =
            adaptation_field_control == 1 ? 4 : 5 + transportPacket[4];
        /* Get the PID from the packet, and check PAT or PMT tables. */
        unsigned short PID = ((transportPacket[1] & 0x1F) << 8) | transportPacket[2];

        if (PID == PAT_PID) {
            AnalyzePAT(&transportPacket[totalHeaderSize],
                       TRANSPORT_PACKET_SIZE - totalHeaderSize);
        } else if (PID == pmt_pid_) {
            if (!pmt_parsed_) {
                AnalyzePMT(&transportPacket[totalHeaderSize],
                           TRANSPORT_PACKET_SIZE - totalHeaderSize);
                pmt_parsed_ = true;

                if (no_more_stream_func_) {
                    no_more_stream_func_(no_more_stream_func_arg_);
                }
            }
        }

        /* Also, if this is the start of a PES packet, then skip over the PES header */
        bool payload_unit_start_indicator = (transportPacket[1] & 0x40) != 0;

        if (payload_unit_start_indicator) {
            /* Note: The following works only for MPEG-2 data. */
            unsigned char PES_header_data_length = transportPacket[totalHeaderSize + 8];
            totalHeaderSize += 9 + PES_header_data_length;

            if (totalHeaderSize >= TRANSPORT_PACKET_SIZE) {
                // Shall not happen
            }
        }

        std::map<unsigned int, void *>::iterator pid_map_it = pid_mempool_map_.find(PID);

        if (pid_map_it != pid_mempool_map_.end()) {
            MemPool *output_mem = static_cast<MemPool *>(pid_map_it->second);

            if (output_mem) {
                /* The remaining data is Video Elementary Stream data. Copy it to the output sink buffer. */
                while (is_running_) {
                    unsigned char *buf_start = output_mem->GetWritePtr();
                    unsigned int free_buf = output_mem->GetFreeFlatBufSize();
                    unsigned int dataSize = TRANSPORT_PACKET_SIZE - totalHeaderSize;

                    if (dataSize > free_buf) {
                        // Demux stuck by decoder.
                        usleep(10 * 1000);
                    } else {
                        memcpy(buf_start, &transportPacket[totalHeaderSize], dataSize);
                        output_mem->UpdateWritePtrCopyData(dataSize);
                        break;
                    }
                }
            } else {
                // assert(0);
            }
        }
    }

    return demuxed_data_size;
}

void MpegTsDemux::AnalyzePAT(unsigned char *pkt, unsigned size)
{
    /* Iterate through to find the PAT to get the PES PID. */
    while (size >= 17) {
        /* Get the Program number. */
        unsigned short program_number = (pkt[9] << 8) | pkt[10];

        if (program_number != 0) {
            pmt_pid_ = ((pkt[11] & 0x1F) << 8) | pkt[12];
            return;
        }

        pkt += 4;
        size -= 4;
    }
}

void MpegTsDemux::AnalyzePMT(unsigned char *pkt, unsigned size)
{
    /* Scan the "elementary_PID"s in the map, until we see the first
     * video stream. */
    /* First, get the "section_length", to get the table's size. */
    unsigned short section_length = ((pkt[2] & 0x0F) << 8) | pkt[3];

    if ((unsigned) (4 + section_length) < size) {
        size = (4 + section_length);
    }

    /* Then, skip any descriptors following the "program_info_length". */
    if (size < 22) {
        return; // not enough data
    }

    unsigned program_info_length = ((pkt[11] & 0x0F) << 8) | pkt[12];
    pkt += 13;
    size -= 13;

    if (size < program_info_length) {
        return; // not enough data
    }

    pkt += program_info_length;
    size -= program_info_length;

    /* Look at each ("stream_type","elementary_PID") pair, looking for a
     *  video stream ("stream_type" == 1 or 2). */
    while (size >= 9) {
        unsigned char stream_type = pkt[0];
        unsigned short elementary_PID = ((pkt[1] & 0x1F) << 8) | pkt[2];
        StreamType new_stream_type = GetStreamType(stream_type);

        if (STREAM_TYPE_OTHER != new_stream_type) {
            std::map<unsigned int, void *>::iterator pid_map_it =
                pid_mempool_map_.find(elementary_PID);

            if (pid_map_it == pid_mempool_map_.end()) {
                if (new_stream_added_func_) {
                    pid_mempool_map_[elementary_PID] =
                        (MemPool *)new_stream_added_func_(new_stream_type,
                                                          new_stream_added_func_arg_);
                }
            }
        } else {
            // Other types
            printf("stream type %d, pid %d, not supported\n", stream_type, elementary_PID);
            //assert(0);
        }

        // TODO:
        // Complete the callback, add pmt parsed done etc
        unsigned short ES_info_length = ((pkt[3] & 0x0F) << 8) | pkt[4];
        pkt += 5;
        size -= 5;

        if (size < ES_info_length) {
            return; // not enough data
        }

        pkt += ES_info_length;
        size -= ES_info_length;
    }
}

StreamType MpegTsDemux::GetStreamType(unsigned ts_stream_type)
{
    switch (ts_stream_type) {
    case 1:
        return STREAM_TYPE_VIDEO_MPEG1;
    case 2:
        return STREAM_TYPE_VIDEO_MPEG2;
    case 0x1b:
        return STREAM_TYPE_VIDEO_AVC;
    case 0xea:
        return STREAM_TYPE_VIDEO_VC1;
    case 3:
        return STREAM_TYPE_AUDIO_MPEG1;
    case 4:
        return STREAM_TYPE_AUDIO_MPEG2;
    case 0x0f:
        return STREAM_TYPE_AUDIO_ADTS;
    case 0x81:
    case 0x06:
        return STREAM_TYPE_AUDIO_AC3;
    case 0x82:
        return STREAM_TYPE_AUDIO_DTS;
    default:
        printf("Stream type 0x%x not supported!!!\n", ts_stream_type);
        break;
    }

    return STREAM_TYPE_OTHER;
}
