#pragma once

#include "bscan/camera.h"

#include "bscan/prim.h"

#include <raylib.h>
#include <rlgl.h>




typedef struct {
    int screenWidth;
    int screenHeight;
    bool shouldClose;

    Buffer target;
 
    Texture2D texture;
} RenderContext;

typedef struct {
    CameraContext* camera;
    RenderContext* render;

    Graph* graph;
    
    Skeleton* skeleton;

} BScanContext;




void bscan_loop(BScanContext* context);