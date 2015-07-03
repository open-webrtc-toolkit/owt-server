/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef MediaEnabling_h
#define MediaEnabling_h

namespace woogeen_base {

class MediaEnabling {
public:
    enum {Enable_NONE = 0x00, Enable_AUDIO = 0x01, Enable_VIDEO = 0x10, Enable_ALL = 0x11};
    MediaEnabling() : m_value(Enable_ALL) { }
    ~MediaEnabling() {}
    const bool hasVideo() const { return m_value & Enable_VIDEO; }
    const bool hasAudio() const { return m_value & Enable_AUDIO; }
    void disableVideo() { m_value &= ~Enable_VIDEO; }
    void disableAudio() { m_value &= ~Enable_AUDIO; }
    void enableVideo() { m_value |= Enable_VIDEO; }
    void enableAudio() { m_value |= Enable_AUDIO; }
private:
    int m_value;
};

} /* namespace woogeen_base */
#endif /* MediaEnabling_h */
