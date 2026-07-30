
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


#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480


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

    context->fd = open(DEVICE, O_RDWR);
    if (context->fd < 0)
        die("open");


    // Check device

    if (xioctl(context->fd, VIDIOC_QUERYCAP, &context->cap) < 0)
        die("VIDIOC_QUERYCAP");

    printf("Camera: %s\n", context->cap.card);


    // Set format

    context->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    context->fmt.fmt.pix.width = WIDTH;
    context->fmt.fmt.pix.height = HEIGHT;
    context->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    context->fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (xioctl(context->fd, VIDIOC_S_FMT, &context->fmt) < 0)
        die("VIDIOC_S_FMT");


    // Request buffers

    context->req.count = BUFFER_COUNT;
    context->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    context->req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(context->fd, VIDIOC_REQBUFS, &context->req) < 0)
        die("VIDIOC_REQBUFS");


    


    // Map buffers
    for (int i = 0; i < context->req.count; i++)
    {
        struct v4l2_buffer buf = {};

        buf.type = context->req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(context->fd, VIDIOC_QUERYBUF, &buf) < 0)
            die("VIDIOC_QUERYBUF");


        context->buffers[i].length = buf.length;

        context->buffers[i].start = mmap(
            NULL,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            context->fd,
            buf.m.offset
        );

        if (context->buffers[i].start == MAP_FAILED)
            die("mmap");
    }


    // Queue buffers
    for (int i = 0; i < context->req.count; i++)
    {
        struct v4l2_buffer buf = {};

        buf.type = context->req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(context->fd, VIDIOC_QBUF, &buf) < 0)
            die("VIDIOC_QBUF");
    }


    // Start streaming
    context->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(context->fd, VIDIOC_STREAMON, &context->type) < 0)
        die("STREAMON");


    printf("Streaming...\n");

    context->output.w = WIDTH;
    context->output.h = HEIGHT;
}

void camera_update(CameraContext* context) {
    struct v4l2_buffer buf = {};

    buf.type = context->type;
    buf.memory = V4L2_MEMORY_MMAP;


    // Wait for frame
    if (xioctl(context->fd, VIDIOC_DQBUF, &buf) < 0)
        die("DQBUF");


    context->output.pixels  = context->buffers[buf.index].start;
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
    if (xioctl(context->fd, VIDIOC_QBUF, &buf) < 0)
        die("QBUF");
}
void camera_cleanup(CameraContext* context) {

    xioctl(context->fd, VIDIOC_STREAMOFF, &context->type);

    close(context->fd);

}
