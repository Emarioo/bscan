
#ifdef _WIN32

#include "bscan/camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define COBJMACROS

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>


typedef struct {
    void *start;
    size_t length;
} CamBuffer;


#define WIDTH  640
#define HEIGHT 480



struct CameraContext_internal {
    IMFAttributes* attrs;
    IMFActivate** devices;
    IMFMediaSource* source;
    IMFSourceReader* reader;
    UINT32 count;

    IMFMediaType* type;

    IMFSample* prevSample;
    IMFMediaBuffer* prevBuffer;
};

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}


static inline UINT64
Pack2UINT32AsUINT64(UINT32 unHigh, UINT32 unLow)
{
    return ((UINT64)unHigh << 32) | unLow;
}

static inline HRESULT
MFSetAttribute2UINT32asUINT64(
    IMFMediaType*  pAttributes,
    REFGUID         guidKey,
    UINT32          unHigh32,
    UINT32          unLow32
    )
{
    return pAttributes->lpVtbl->SetUINT64(pAttributes, guidKey, Pack2UINT32AsUINT64(unHigh32, unLow32));
}

static inline HRESULT
MFSetAttributeSize(
    IMFMediaType*  pAttributes,
    REFGUID         guidKey,
    UINT32          unWidth,
    UINT32          unHeight
    )
{
    return MFSetAttribute2UINT32asUINT64(pAttributes, guidKey, unWidth, unHeight);
}

void camera_init(CameraContext* context) {
    memset(context, 0, sizeof(*context));
    CameraContext_internal* internal = context->internal = calloc(1, sizeof(CameraContext_internal));
    memset(internal, 0, sizeof(*internal));

    
    HRESULT hr;



    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        die("startup failed\n");
    }

    //-----------------------------------------
    // Enumerate webcams
    //-----------------------------------------

    MFCreateAttributes(&internal->attrs, 1);

    IMFAttributes_SetGUID(
        internal->attrs,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    MFEnumDeviceSources(internal->attrs, &internal->devices, &internal->count);

    if (internal->count == 0)
    {
        die("No webcam found\n");
    }

    //-----------------------------------------
    // Open first webcam
    //-----------------------------------------

    IMFActivate_ActivateObject(
        internal->devices[0],
        &IID_IMFMediaSource,
        (void**)&internal->source);

    MFCreateSourceReaderFromMediaSource(
        internal->source,
        NULL,
        &internal->reader);

    //-----------------------------------------
    // Request YUY2 640x480
    //-----------------------------------------


    MFCreateMediaType(&internal->type);

    IMFMediaType_SetGUID(internal->type,
        &MF_MT_MAJOR_TYPE,
        &MFMediaType_Video);

    IMFMediaType_SetGUID(internal->type,
        &MF_MT_SUBTYPE,
        &MFVideoFormat_YUY2);

    MFSetAttributeSize(internal->type, &MF_MT_FRAME_SIZE, WIDTH, HEIGHT);

    IMFSourceReader_SetCurrentMediaType(
        internal->reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        NULL,
        internal->type);

    IMFMediaType_Release(internal->type);


    // printf("Streaming...\n");

    context->output.w = WIDTH;
    context->output.h = HEIGHT;
}

void camera_update(CameraContext* context) {
    CameraContext_internal* internal = context->internal;


    DWORD streamIndex;
    DWORD flags;
    LONGLONG timestamp;

    if (internal->prevSample) {
        IMFMediaBuffer_Unlock(internal->prevBuffer);
        IMFMediaBuffer_Release(internal->prevBuffer);
        IMFSample_Release(internal->prevSample);
    }

    IMFSourceReader_ReadSample(
        internal->reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &timestamp,
        &internal->prevSample);

    if (!internal->prevSample) {
        // use blank template, print warning?
        context->output.pixels  = NULL;
        return;
    }
    internal->prevSample->lpVtbl->ConvertToContiguousBuffer(internal->prevSample, &internal->prevBuffer);

    BYTE* pixels;
    DWORD maxLength;
    DWORD currentLength;

    IMFMediaBuffer_Lock(
        internal->prevBuffer,
        &pixels,
        &maxLength,
        &currentLength);

    // printf("Frame size = %lu bytes\n", currentLength);

    context->output.pixels  = (void*)pixels;

    // pixels points to the raw YUY2 image.
    // Size should be:
    // width * height * 2
    //
    // 640*480*2 = 614400 bytes

    // Example:
    // printf("%u %u %u %u\n",
    //     pixels[0],
    //     pixels[1],
    //     pixels[2],
    //     pixels[3]);

    /*
        YUYV format:

        byte 0: Y0
        byte 1: U
        byte 2: Y1
        byte 3: V

        Two pixels = 4 bytes
    */

    // printf("Frame size: %d bytes\n", buf.bytesused);

}
void camera_cleanup(CameraContext* context) {
    CameraContext_internal* internal = context->internal;


    IMFSourceReader_Release(internal->reader);
    IMFMediaSource_Release(internal->source);

    for (UINT32 i = 0; i < internal->count; i++)
        IMFActivate_Release(internal->devices[i]);

    // CoTaskMemFree(devices);

    IMFAttributes_Release(internal->attrs);

    MFShutdown();
    

}

#endif // _WIN32