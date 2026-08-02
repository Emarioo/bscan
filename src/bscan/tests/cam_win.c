#ifdef _WIN32

#define COBJMACROS

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <stdio.h>

// #pragma comment(lib, "mfplat.lib")
// #pragma comment(lib, "mf.lib")
// #pragma comment(lib, "mfreadwrite.lib")
// #pragma comment(lib, "mfuuid.lib")

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

#define WIDTH  640
#define HEIGHT 480

int cam_test(void)
{
    HRESULT hr;

    IMFAttributes* attrs = NULL;
    IMFActivate** devices = NULL;
    IMFMediaSource* source = NULL;
    IMFSourceReader* reader = NULL;

    UINT32 count = 0;

    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        printf("startup failed\n");
        return 1;
    }

    //-----------------------------------------
    // Enumerate webcams
    //-----------------------------------------

    MFCreateAttributes(&attrs, 1);

    IMFAttributes_SetGUID(
        attrs,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    MFEnumDeviceSources(attrs, &devices, &count);

    if (count == 0)
    {
        printf("No webcam found\n");
        return 1;
    }

    //-----------------------------------------
    // Open first webcam
    //-----------------------------------------

    IMFActivate_ActivateObject(
        devices[0],
        &IID_IMFMediaSource,
        (void**)&source);

    MFCreateSourceReaderFromMediaSource(
        source,
        NULL,
        &reader);

    //-----------------------------------------
    // Request YUY2 640x480
    //-----------------------------------------

    IMFMediaType* type = NULL;

    MFCreateMediaType(&type);

    IMFMediaType_SetGUID(type,
        &MF_MT_MAJOR_TYPE,
        &MFMediaType_Video);

    IMFMediaType_SetGUID(type,
        &MF_MT_SUBTYPE,
        &MFVideoFormat_YUY2);

    MFSetAttributeSize(type, &MF_MT_FRAME_SIZE, WIDTH, HEIGHT);

    IMFSourceReader_SetCurrentMediaType(
        reader,
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        NULL,
        type);

    IMFMediaType_Release(type);

    //-----------------------------------------
    // Capture one frame
    //-----------------------------------------

    IMFSample* sample = NULL;
    IMFMediaBuffer* buffer = NULL;

    DWORD streamIndex;
    DWORD flags;
    LONGLONG timestamp;

    while (1) {

        IMFSourceReader_ReadSample(
            reader,
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &streamIndex,
            &flags,
            &timestamp,
            &sample);

        if (sample)
        {
            sample->lpVtbl->ConvertToContiguousBuffer(sample, &buffer);

            BYTE* pixels;
            DWORD maxLength;
            DWORD currentLength;

            IMFMediaBuffer_Lock(
                buffer,
                &pixels,
                &maxLength,
                &currentLength);

            printf("Frame size = %lu bytes\n", currentLength);

            // pixels points to the raw YUY2 image.
            // Size should be:
            // width * height * 2
            //
            // 640*480*2 = 614400 bytes

            // Example:
            printf("%u %u %u %u\n",
                pixels[0],
                pixels[1],
                pixels[2],
                pixels[3]);

            IMFMediaBuffer_Unlock(buffer);
            IMFMediaBuffer_Release(buffer);
            IMFSample_Release(sample);
        }
    }

    //-----------------------------------------
    // Cleanup
    //-----------------------------------------

    IMFSourceReader_Release(reader);
    IMFMediaSource_Release(source);

    for (UINT32 i = 0; i < count; i++)
        IMFActivate_Release(devices[i]);

    // CoTaskMemFree(devices);

    IMFAttributes_Release(attrs);

    MFShutdown();

    return 0;
}

#endif