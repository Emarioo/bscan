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
#define BUFFER_COUNT 4

typedef struct
{
    void *start;
    size_t length;
} Buffer;

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


int cam_test()
{
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0)
        die("open");


    // Check device
    struct v4l2_capability cap = {};

    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
        die("VIDIOC_QUERYCAP");

    printf("Camera: %s\n", cap.card);


    // Set format
    struct v4l2_format fmt = {};

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
        die("VIDIOC_S_FMT");


    // Request buffers
    struct v4l2_requestbuffers req = {};

    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
        die("VIDIOC_REQBUFS");


    Buffer buffers[BUFFER_COUNT];


    // Map buffers
    for (int i = 0; i < req.count; i++)
    {
        struct v4l2_buffer buf = {};

        buf.type = req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
            die("VIDIOC_QUERYBUF");


        buffers[i].length = buf.length;

        buffers[i].start = mmap(
            NULL,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            buf.m.offset
        );

        if (buffers[i].start == MAP_FAILED)
            die("mmap");
    }


    // Queue buffers
    for (int i = 0; i < req.count; i++)
    {
        struct v4l2_buffer buf = {};

        buf.type = req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
            die("VIDIOC_QBUF");
    }


    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
        die("STREAMON");


    printf("Streaming...\n");


    while (1)
    {
        struct v4l2_buffer buf = {};

        buf.type = type;
        buf.memory = V4L2_MEMORY_MMAP;


        // Wait for frame
        if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0)
            die("DQBUF");


        unsigned char *pixels = buffers[buf.index].start;


        /*
            YUYV format:

            byte 0: Y0
            byte 1: U
            byte 2: Y1
            byte 3: V

            Two pixels = 4 bytes

            Do your processing here.
        */

        printf("Frame size: %d bytes\n", buf.bytesused);


        // Return buffer
        if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
            die("QBUF");
    }


    xioctl(fd, VIDIOC_STREAMOFF, &type);

    close(fd);

    return 0;
}