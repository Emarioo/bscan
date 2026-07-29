#pragma once

#include "bscan/prim.h"

#include <linux/videodev2.h>

#define BUFFER_COUNT 4


typedef struct {
    void *start;
    size_t length;
} CamBuffer;

typedef struct {
    int fd;
    struct v4l2_capability cap;
    enum v4l2_buf_type type;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    CamBuffer buffers[BUFFER_COUNT];

    Buffer output;
} CameraContext;

void camera_init(CameraContext* context);
void camera_update(CameraContext* context);
void camera_cleanup(CameraContext* context);
