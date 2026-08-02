
#ifndef _WIN32

#include "bscan/camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

typedef struct {
    void *start;
    size_t length;
} CamBuffer;


#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480

#define BUFFER_COUNT 4



struct CameraContext_internal {
    int fd;
    struct v4l2_capability cap;
    enum v4l2_buf_type type;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    CamBuffer buffers[BUFFER_COUNT];
};

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

static int xioctl(int fd, unsigned long request, void *arg)
{
    int r;

    do
    {
        r = ioctl(fd, request, arg);
    }
    while (r == -1 && errno == EINTR);

    return r;
}


void camera_init(CameraContext* context) {
    memset(context, 0, sizeof(*context));
    CameraContext_internal* internal = context->internal = calloc(1, sizeof(CameraContext_internal));
    memset(internal, 0, sizeof(*internal));

    internal->fd = open(DEVICE, O_RDWR);
    if (internal->fd < 0)
        die("open");


    // Check device

    if (xioctl(internal->fd, VIDIOC_QUERYCAP, &internal->cap) < 0)
        die("VIDIOC_QUERYCAP");

    printf("Camera: %s\n", internal->cap.card);


    // Set format

    internal->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    internal->fmt.fmt.pix.width = WIDTH;
    internal->fmt.fmt.pix.height = HEIGHT;
    internal->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    internal->fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (xioctl(internal->fd, VIDIOC_S_FMT, &internal->fmt) < 0)
        die("VIDIOC_S_FMT");


    // Request buffers

    internal->req.count = BUFFER_COUNT;
    internal->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    internal->req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(internal->fd, VIDIOC_REQBUFS, &internal->req) < 0)
        die("VIDIOC_REQBUFS");


    


    // Map buffers
    for (int i = 0; i < internal->req.count; i++)
    {
        struct v4l2_buffer buf = {};

        buf.type = internal->req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(internal->fd, VIDIOC_QUERYBUF, &buf) < 0)
            die("VIDIOC_QUERYBUF");


        internal->buffers[i].length = buf.length;

        internal->buffers[i].start = mmap(
            NULL,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            internal->fd,
            buf.m.offset
        );

        if (internal->buffers[i].start == MAP_FAILED)
            die("mmap");
    }


    // Queue buffers
    for (int i = 0; i < internal->req.count; i++)
    {
        struct v4l2_buffer buf = {};

        buf.type = internal->req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(internal->fd, VIDIOC_QBUF, &buf) < 0)
            die("VIDIOC_QBUF");
    }


    // Start streaming
    internal->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(internal->fd, VIDIOC_STREAMON, &internal->type) < 0)
        die("STREAMON");


    printf("Streaming...\n");

    context->output.w = WIDTH;
    context->output.h = HEIGHT;
}

void camera_update(CameraContext* context) {
    struct v4l2_buffer buf = {};

    CameraContext_internal* internal = context->internal;


    buf.type = internal->type;
    buf.memory = V4L2_MEMORY_MMAP;


    // Wait for frame
    if (xioctl(internal->fd, VIDIOC_DQBUF, &buf) < 0)
        die("DQBUF");


    context->output.pixels  = internal->buffers[buf.index].start;
    /*
        YUYV format:

        byte 0: Y0
        byte 1: U
        byte 2: Y1
        byte 3: V

        Two pixels = 4 bytes
    */

    // printf("Frame size: %d bytes\n", buf.bytesused);


    // Return buffer
    if (xioctl(internal->fd, VIDIOC_QBUF, &buf) < 0)
        die("QBUF");
}
void camera_cleanup(CameraContext* context) {
    CameraContext_internal* internal = context->internal;

    xioctl(internal->fd, VIDIOC_STREAMOFF, &internal->type);

    close(internal->fd);

}



#endif // _WIN32