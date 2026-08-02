#pragma once

#include "bscan/buffer.h"



typedef struct CameraContext_internal CameraContext_internal;

typedef struct {
    Buffer output;

    CameraContext_internal* internal;
} CameraContext;

void camera_init(CameraContext* context);
void camera_update(CameraContext* context);
void camera_cleanup(CameraContext* context);
