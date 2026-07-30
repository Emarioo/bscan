
#include "bscan/bscan.h"



#include "raylib.h"
#include "rlgl.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define ABS(X) ((X) < 0 ? -(X) : (X))


void render_init(RenderContext* render);
void render_update(RenderContext* render);
void render_cleanup(RenderContext* render);

Buffer make_buffer(BScanContext* context);
Point* make_point(BScanContext* context, int x, int y);
Edge* make_edge(BScanContext* context, Point* a, Point* b);

#define ENCODE_RGB(W,R,G,B) W = ((word)(R)) | ((word)(G) << 8) | ((word)(B) << 16) | 0xFF000000;
#define DECODE_RGB(W,R,G,B) ( R = ((W)) & 0xFF, G = ((W) >> 8) & 0xFF, B = ((W) >> 16) & 0xFF )

#define BRIGHTNESS (0.2126*R + 0.7152*G + 0.0722*B)


void yuv_to_rgb(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int pw_half = src_buf.w/2;
    int ph = src_buf.h;

    #define CLAMP(X) ( (X) < 0 ? 0 : ((X) > 255 ? 255 : (X) ) )

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw_half; x++) {
            word W = src[x + y * pw_half];

            int Y0 = (W >> 0) & 0xFF;
            int U  = (W >> 8) & 0xFF;
            int Y1 = (W >> 16) & 0xFF;
            int V  = (W >> 24) & 0xFF;

            int R0 = Y0 + 1.402 * (V - 128);
            int G0 = Y0 - 0.344 * (U - 128) - 0.714 * (V - 128);
            int B0 = Y0 + 1.772 * (U - 128);
            int R1 = Y1 + 1.402 * (V - 128);
            int G1 = Y1 - 0.344 * (U - 128) - 0.714 * (V - 128);
            int B1 = Y1 + 1.772 * (U - 128);
            R0 = CLAMP(R0);
            G0 = CLAMP(G0);
            B0 = CLAMP(B0);
            R1 = CLAMP(R1);
            G1 = CLAMP(G1);
            B1 = CLAMP(B1);
            
            word W0, W1;
            ENCODE_RGB(W0, R0, G0, B0);
            ENCODE_RGB(W1, R1, G1, B1);

            dst[2 * x + y * pw]    = W0;
            dst[2 * x +1 + y * pw] = W1;
        }
    }
}


void grayscale(Buffer dst_buf, Buffer src_buf) {
    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B;

            W = src[x + y * pw];
            DECODE_RGB(W, R,G,B);
            int brightness = BRIGHTNESS;
            brightness = CLAMP(brightness);
            ENCODE_RGB(W, brightness,brightness,brightness);

            dst[x + y * pw] = W;
        }
    }
}

void blur(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int sR = 0, sG = 0, sB = 0;
            int R,G,B;

            #define BLUR_SUM(SX,SY) \
                W = src[(x + SX) + (y+SY) * pw]; \
                DECODE_RGB(W, R, G, B); \
                sR += R; \
                sG += G; \
                sB += B; 

            BLUR_SUM(-1,-1)
            BLUR_SUM(0, -1)
            BLUR_SUM(1, -1)
            BLUR_SUM(-1, 0)
            BLUR_SUM(0,  0)
            BLUR_SUM(1,  0)
            BLUR_SUM(-1, 1)
            BLUR_SUM(0,  1)
            BLUR_SUM(1,  1)

            ENCODE_RGB(W, sR/9, sG/9, sB/9);

            dst[x + y * pw] = W;
        }
    }
}


void gaus5x5(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int sR = 0, sG = 0, sB = 0;
            int R,G,B;

            #define GAUS_SUM(SX,SY,WEIGHT) \
                W = src[(x + SX) + (y+SY) * pw]; \
                DECODE_RGB(W, R, G, B); \
                sR += R*WEIGHT; \
                sG += G*WEIGHT; \
                sB += B*WEIGHT; 

            GAUS_SUM(-2, -2, 1)
            GAUS_SUM(-1, -2, 4)
            GAUS_SUM( 0, -2, 7)
            GAUS_SUM(-1, -2, 4)
            GAUS_SUM(-2, -2, 1)

            GAUS_SUM(-2, -1, 4)
            GAUS_SUM(-1, -1, 16)
            GAUS_SUM( 0, -1, 26)
            GAUS_SUM(-1, -1, 16)
            GAUS_SUM(-2, -1, 4)

            GAUS_SUM(-2, 0, 7)
            GAUS_SUM(-1, 0, 26)
            GAUS_SUM( 0, 0, 41)
            GAUS_SUM(-1, 0, 26)
            GAUS_SUM(-2, 0, 7)

            GAUS_SUM(-2, -1, 4)
            GAUS_SUM(-1, -1, 16)
            GAUS_SUM( 0, -1, 26)
            GAUS_SUM(-1, -1, 16)
            GAUS_SUM(-2, -1, 4)

            GAUS_SUM(-2, -2, 1)
            GAUS_SUM(-1, -2, 4)
            GAUS_SUM( 0, -2, 7)
            GAUS_SUM(-1, -2, 4)
            GAUS_SUM(-2, -2, 1)

            int weight = 273;

            ENCODE_RGB(W, sR/weight, sG/weight, sB/weight);

            dst[x + y * pw] = W;
        }
    }
}

void gaus3x3(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int sR = 0, sG = 0, sB = 0;
            int R,G,B;

            #define GAUS_SUM(SX,SY,WEIGHT) \
                W = src[(x + SX) + (y+SY) * pw]; \
                DECODE_RGB(W, R, G, B); \
                sR += R*WEIGHT; \
                sG += G*WEIGHT; \
                sB += B*WEIGHT; 

            GAUS_SUM(-1, -1, 1)
            GAUS_SUM( 0, -1, 2)
            GAUS_SUM( 1, -1, 1)

            GAUS_SUM(-1, 0, 2)
            GAUS_SUM( 0, 0, 4)
            GAUS_SUM( 1, 0, 2)

            GAUS_SUM(-1, 1, 1)
            GAUS_SUM( 0, 1, 2)
            GAUS_SUM( 1, 1, 1)

            int weight = 16;

            ENCODE_RGB(W, sR/weight, sG/weight, sB/weight);

            dst[x + y * pw] = W;
        }
    }
}
#define THRESHOLD 50


void edge(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    #define BRIGHTNESS (0.2126*R + 0.7152*G + 0.0722*B)

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word sW, W;
            int R,G,B;
            int sumDiff = 0;
            int sumTotal = 0;
            int sInt;
            int dInt;

            sW = W = src[x + y * pw];
            DECODE_RGB(W, R,G,B);
            sInt = BRIGHTNESS;

            #define CALC(SX,SY) \
                W = ((word*)src)[(x + SX) + (y+SY) * pw];  \
                DECODE_RGB(W, R,G,B);          \
                dInt = BRIGHTNESS;             \
                sumDiff += ABS(dInt - sInt);   \
                sumTotal += dInt - sInt;

            // CALC(-2,-2)
            CALC(-1,-1)
            CALC(0, -1)
            CALC(1, -1)
            CALC(-1, 0)
            // CALC(2, -2)
            // CALC(0,  0)
            CALC(1,  0)
            CALC(-1, 1)
            // CALC(-2, 2)
            CALC(0,  1)
            CALC(1,  1)
            // CALC(2,  2)

            int value = sumDiff;

            // dst[x + y * pw] = CLAMP(sumDiff);
            // dst[x + y * pw] = sumDiff < ABS(sumTotal) ? 0 : sumDiff;
            // dst[x + y * pw] = ABS(sumTotal);
            // dst[x + y * pw] = sInt;
            // dst[x + y * pw] = (((sumDiff & 0xFF) << 16)) + sW;


            // strength = BRIGHTNESS;

            // if (value > THRESHOLD)
            //     value = 0xFF00 | value;
                // value = 0x0000FF00;

            dst[x + y * pw] = value;
        }
    }
}


void point_sum(BScanContext* context, Buffer src_buf) {

    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;


    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B;
            int strength;
            W = src[x + y * pw];
            DECODE_RGB(W, R,G,B);
            strength = R;
            // strength = BRIGHTNESS;

            if (strength < THRESHOLD)
                continue;

            make_point(context, x, y);
        }
    }
}


void point_reduce(BScanContext* context) {

    int newPointLen = 0;
    Graph* graph = context->graph;

    const int threshold = 5;

    for (int pi=0;pi<graph->points_len;pi++) {
        Point* a = &graph->points[pi];

        bool anyClose = false;
        for (int i=pi+1;i<graph->points_len;i++) {
            Point* b = &graph->points[i];

            int dx = a->x - b->x;
            int dy = a->y - b->y;

            int dist = dx*dx + dy*dy;
            if (dist < threshold) {
                anyClose = true;
                break;
            }
        }
        
        if (!anyClose) {
            Point* dst = &graph->points[newPointLen];
            *dst = *a;
            newPointLen++;
        }
        
    }

    graph->points_len = newPointLen;
}


void edge_sum(BScanContext* context) {

    Graph* graph = context->graph;

    const int threshold = 5;

    for (int pi=0;pi<graph->points_len;pi++) {
        Point* a = &graph->points[pi];
        Point* closest = NULL;
        int closestDist = 9999;

        for (int i=pi+1;i<graph->points_len;i++) {
            Point* b = &graph->points[i];

            int dx = a->x - b->x;
            int dy = a->y - b->y;
            int dist = dx*dx + dy*dy;

            if (dist < closestDist && dist < 40*40) {
                closestDist = dist;
                closest = b;
            }
        }

        if (closest)
            make_edge(context, a, closest);
    }
}


void prepare_skeleton(Skeleton* skel) {
    
    skel->bones_len = BONE_COUNT;


    // Center bones

    skel->bones[BONE_TORSO] = (Bone){
        .parent = 0,
        .localPos = (Vector3){0, 0.9f, 0},
        .localRot = QuaternionIdentity(),
    };

    skel->bones[BONE_HEAD] = (Bone){
        .parent = BONE_TORSO,
        .localPos = (Vector3){0, 0.24f, 0},
        .localRot = QuaternionIdentity(),
    };

    // Left bones

    skel->bones[BONE_LEFT_SHOULDER] = (Bone){
        .parent = BONE_TORSO,
        .localPos = {-0.25f, 0.05f, 0},
        .localRot = QuaternionIdentity(),
    };

    skel->bones[BONE_LEFT_ARM] = (Bone){
        .parent = BONE_LEFT_SHOULDER,
        .localPos = {0, -0.30f, 0},
        .localRot = QuaternionIdentity(),
    };
    skel->bones[BONE_LEFT_WRIST] = (Bone){
        .parent = BONE_LEFT_ARM,
        .localPos = {0, -0.30f, 0},
        .localRot = QuaternionIdentity(),
    };

    skel->bones[BONE_LEFT_HIP] = (Bone){
        .parent = BONE_TORSO,
        .localPos = {-0.15, -0.53f, 0},
        .localRot = QuaternionIdentity(),
    };

    skel->bones[BONE_LEFT_KNEE] = (Bone){
        .parent = BONE_LEFT_HIP,
        .localPos = {0, -0.3f, 0},
        .localRot = QuaternionIdentity(),
    };

    skel->bones[BONE_LEFT_ANKLE] = (Bone){
        .parent = BONE_LEFT_KNEE,
        .localPos = {0, -0.3f, 0.0},
        .localRot = QuaternionIdentity(),
    };

    // Right bones

    int leftToRightStride = BONE_RIGHT_SHOULDER - BONE_LEFT_SHOULDER;
    for (int i = 0; i < leftToRightStride; i++) {
        Bone* left = &skel->bones[BONE_LEFT_SHOULDER + i];
        Bone* right = &skel->bones[BONE_RIGHT_SHOULDER + i];
        *right = *left;
        if (right->parent >= BONE_LEFT_SHOULDER)
            right->parent += leftToRightStride;
        right->localPos.x = -right->localPos.x;
        right->localRot.x = -right->localRot.x;
    }
}

void compute_skeleton(Skeleton* skel) {

    for (int i=0;i<skel->bones_len;i++) {
        Bone* bone = &skel->bones[i];
        Bone* parent = i == 0 ? NULL : &skel->bones[bone->parent];

        if (!parent) {
            bone->worldRot = bone->localRot;
            bone->worldPos = bone->localPos;
            continue;
        }

        Vector3 offset = Vector3RotateByQuaternion(bone->localPos, parent->worldRot);
        bone->worldPos = Vector3Add(parent->worldPos, offset);
        bone->worldRot = QuaternionMultiply(parent->worldRot, bone->localRot);
    }
}

void bscan_loop(BScanContext* context) {

    CameraContext* camera   = context->camera   = calloc(1, sizeof(CameraContext));
    RenderContext* render   = context->render   = calloc(1, sizeof(RenderContext));
    Graph*         graph    = context->graph    = calloc(1, sizeof(Graph));
    Skeleton*      skeleton = context->skeleton = calloc(1, sizeof(Skeleton));

    prepare_skeleton(skeleton);

    camera_init(context->camera);

    render_init(context->render);

    Buffer temp0 = make_buffer(context);
    Buffer temp1 = make_buffer(context);
    render->target = temp1;

    Image img = {
        .data = render->target.pixels,
        .width = render->target.w,
        .height = render->target.h,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };

    render->texture = LoadTextureFromImage(img);

    graph->edges_cap = 100000;
    graph->edges = malloc(graph->edges_cap * sizeof(Edge));
    graph->points_cap = 100000;
    graph->points = malloc(graph->points_cap * sizeof(Point));


    // Camera raycam = { { 0.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    Camera raycam = { 0 };
    raycam.position = (Vector3){ 0.0f, 1.2f, -5.0f };    // Camera position
    // raycam.target = (Vector3){ 0.185f, 0.4f, 0.0f };    // Camera looking at point
    raycam.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    raycam.fovy = 45.0f;                                // Camera field-of-view Y
    raycam.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    DisableCursor();                // Limit cursor to relative movement inside the window


    while (!WindowShouldClose()) {
        
        camera_update(context->camera);

        Buffer tmp;
        Buffer src = temp0;
        Buffer dst = temp1;


        #define PIPE_START(F, INPUT) F(dst, INPUT); tmp = src; src = dst; dst = tmp;
        #define PIPE(F) F(dst, src); tmp = src; src = dst; dst = tmp;

        PIPE_START(yuv_to_rgb, camera->output)
        // PIPE(grayscale)
        // PIPE(blur)
        // PIPE(gaus3x3)
        // PIPE(gaus5x5)
        PIPE(edge)


        graph->points_len = 0;
        graph->edges_len = 0;

        point_sum(context, src);

        point_reduce(context);

        edge_sum(context);

        printf("%d\n", graph->points_len);

        render->target = src;


        UpdateCamera(&raycam, CAMERA_FREE);

        BeginDrawing();

        ClearBackground(RAYWHITE);

        rlDisableColorBlend();
        

        UpdateTexture(render->texture, render->target.pixels);

            
        // @TODO Upscaled frame buffer.
        DrawTexture(render->texture, 0, 0, WHITE);


        for (int i=0;i<graph->points_len;i++) {
            Point* p = &graph->points[i];
            DrawPixel(p->x, p->y, WHITE);
        }


        for (int i=0;i<graph->edges_len;i++) {
            Point* a = graph->edges[i].a;
            Point* b = graph->edges[i].b;
            DrawLine(a->x, a->y, b->x, b->y, PURPLE);
        }

        compute_skeleton(skeleton);
        
        BeginMode3D(raycam);

            // DrawCube((Vector3){0,0,0}, 1, 1, 1, WHITE);

            DrawGrid(10, 1.0f);        // Draw a grid

            for (int i = 0; i < skeleton->bones_len; i++) {
                Bone* b = &skeleton->bones[i];

                DrawSphere(b->worldPos, 0.03f, RED);

                if (i == 0)
                    continue;

                Bone* p = &skeleton->bones[b->parent];

                DrawLine3D(
                    p->worldPos,
                    b->worldPos,
                    BLUE);
            }


        EndMode3D();
            

        EndDrawing();

    }

    render_cleanup(context->render);
    camera_cleanup(context->camera);

}


void render_init(RenderContext* render) {
    render->screenWidth = 800;
    render->screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(render->screenWidth, render->screenHeight, "Body Scanner");

    SetTargetFPS(60);

}

void render_update(RenderContext* render) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    rlDisableColorBlend();
    

    UpdateTexture(render->texture, render->target.pixels);

        
    // @TODO Upscaled frame buffer.
    DrawTexture(render->texture, 0, 0, WHITE);
        

    EndDrawing();

    render->shouldClose = WindowShouldClose();
}

void render_cleanup(RenderContext* render) {

    UnloadTexture(render->texture);

    CloseWindow();
}

Buffer make_buffer(BScanContext* context) {
    RenderContext* render = context->render;
    int w = context->camera->output.w;
    int h = context->camera->output.h;
    int extra_padding = 2;
    Buffer buffer = { .w = w, .h = h };
    buffer.pixels = malloc(w * (h + 2*extra_padding) * PIXEL_SIZE);
    buffer.pixels += extra_padding * w + 20;
    return buffer;
}


Point* make_point(BScanContext* context, int x, int y) {
    Point* p = &context->graph->points[context->graph->points_len++];
    p->x = x;
    p->y = y;
    return p;
}
Edge* make_edge(BScanContext* context, Point* a, Point* b) {
    Edge* edge = &context->graph->edges[context->graph->edges_len++];
    edge->a = a;
    edge->b = b;
    return edge;
}