
#include "bscan/bscan.h"



#include "raylib.h"
#include "rlgl.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define ABS(X) ((X) < 0 ? -(X) : (X))


void render_init(RenderContext* render);
void render_update(RenderContext* render);
void render_cleanup(RenderContext* render);

Buffer make_buffer(BScanContext* context);


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


void gaus(Buffer dst_buf, Buffer src_buf) {

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

            // GAUS_SUM(-1,-1, 1)
            // GAUS_SUM(0, -1, 2)
            // GAUS_SUM(1, -1, 1)
            // GAUS_SUM(-1, 0, 2)
            // GAUS_SUM(0,  0, 4)
            // GAUS_SUM(1,  0, 2)
            // GAUS_SUM(-1, 1, 1)
            // GAUS_SUM(0,  1, 2)
            // GAUS_SUM(1,  1, 1)

            GAUS_SUM(-1,-1, 1)
            GAUS_SUM(0,-1, 4)
            GAUS_SUM(1,-1, 1)

            GAUS_SUM(-1,0, 4)
            GAUS_SUM(0,0,16)
            GAUS_SUM(1,0,4)

            GAUS_SUM(-1,1,1)
            GAUS_SUM(0,1,4)
            GAUS_SUM(1,1,1)

            int weight = 36;
            // int weight = 16;

            ENCODE_RGB(W, sR/weight, sG/weight, sB/weight);

            dst[x + y * pw] = W;
        }
    }
}

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

            // dst[x + y * pw] = CLAMP(sumDiff);
            dst[x + y * pw] = sumDiff;
            // dst[x + y * pw] = sumDiff < ABS(sumTotal) ? 0 : sumDiff;
            // dst[x + y * pw] = ABS(sumTotal);
            // dst[x + y * pw] = sInt;
            // dst[x + y * pw] = (((sumDiff & 0xFF) << 16)) + sW;
        }
    }
}

void bscan_loop(BScanContext* context) {

    CameraContext* camera = context->camera = calloc(1, sizeof(CameraContext));
    RenderContext* render = context->render = calloc(1, sizeof(RenderContext));

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



    while (!context->render->shouldClose) {
        
        camera_update(context->camera);

        Buffer tmp;
        Buffer src = temp0;
        Buffer dst = temp1;


        #define PIPE_START(F, INPUT) F(dst, INPUT); tmp = src; src = dst; dst = tmp;
        #define PIPE(F) F(dst, src); tmp = src; src = dst; dst = tmp;

        PIPE_START(yuv_to_rgb, camera->output)
        // PIPE(grayscale)
        PIPE(blur)
        // PIPE(gaus)
        PIPE(edge)

        render->target = src;

        render_update(context->render);   

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
    Buffer buffer = { .w = w, .h = h };
    buffer.pixels = malloc(w * (h + 3) * PIXEL_SIZE);
    buffer.pixels += w + 20;
    return buffer;
}
