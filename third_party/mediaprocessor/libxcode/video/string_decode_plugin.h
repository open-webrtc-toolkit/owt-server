/* <COPYRIGHT_TAG>*/
//#pragma once

#ifndef STRING_DECODER_PLUGIN_H_
#define STRING_DECODER_PLUGIN_H_

#ifdef ENABLE_STRING_CODEC
#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "base/media_common.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TYPES_H

//Task structure for plugin
typedef struct {
    StringInfo        *stringInfo; //Pointer to string info
    mfxFrameSurface1  *surfaceOut; //Pointer to output surface
    bool              bBusy;      //Task is in use or not
} MFXTask2;

//
// This class uses the FreeType and the Media SDK plug-in interfaces to
// realize an asynchronous string processing user plug-in which can be used
// in a transcode pipeline.
//
class StringDecPlugin : public MFXGenericPlugin
{
public:
    StringDecPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus Init(mfxVideoParam *param);
    virtual mfxStatus DecodeHeader(StringInfo *strinfo, mfxVideoParam *par);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Close();
    virtual ~StringDecPlugin();
    mfxExtBuffer *GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

    // Interface called by Media SDK framework
    mfxStatus PluginInit(mfxCoreInterface *core);
    mfxStatus PluginClose();
    mfxStatus GetPluginParam(mfxPluginParam *par);
    mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task); //called to submit task but not execute it
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a); //function that called to start task execution and check status of execution
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts); //free task and releated resources
    mfxStatus SetAuxParams(void *auxParam, int auxParamSize);
    void      Release();

private:
    // Interface for string rendering
    mfxStatus InitFreeType(StringInfo *strinfo);
    mfxStatus RenderString(StringInfo *stringInfo);
    mfxStatus Bmp2Yuv(char **text_data, int bmp_width, int bmp_height, StringInfo *strinfo);
    mfxStatus UploadToSurface(unsigned char *yuv_buf, mfxFrameSurface1 *surface_out);
    mfxStatus RelFreeType();

protected:
    mfxCoreInterface  *m_pmfxCore;        //Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam     m_mfxPluginParam;  //Plug-in parameters
    mfxU16             m_MaxNumTasks;     //Max number of concurrent tasks
    MFXTask2          *m_pTasks;          //Array of tasks
    mfxVideoParam      m_VideoParam;
    mfxFrameAllocator *m_pAlloc;
    mfxExtBuffer      *m_ExtParam;
    bool               m_bIsOpaque;

    //String Decoder
    FT_Library     freetype_library_; //an instance of FreeType library
    FT_Face        font_face_;        //an instance of font face
    bool           freetype_init_;    //whether freetype inited
    int            bmp_width_;        //triple of width of bitmap
    int            bmp_height_;       //height of bitmap
    char         **text_data_;
    unsigned char *yuv_buf_;         //point to yuv buffer
    StringInfo    *string_info_;      //string info
};
#endif
#endif /* STRING_DECODER_PLUGIN_H_ */
