
#include "bscan/bscan.h"



#include "raylib.h"
#include "rlgl.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define ABS(X) ((X) < 0 ? -(X) : (X))
#define ARRAY_LENGTH(A) (sizeof(A)/sizeof(*A))

#define ASSERT(EXPR) ((EXPR) ? true : (fprintf(stderr,"[ASSERT] %s (%s:%u)\n",#EXPR,__FILE__,__LINE__), *((char*)0) = 0))


#include <math.h>

static inline void rgb_to_hsv(
    float r, float g, float b,
    float *h, float *s, float *v)
{
    float max = fmaxf(r, fmaxf(g, b));
    float min = fminf(r, fminf(g, b));
    float delta = max - min;

    *v = max;

    if (max <= 0.0f)
    {
        *h = 0.0f;
        *s = 0.0f;
        return;
    }

    *s = delta / max;

    if (delta == 0.0f)
    {
        *h = 0.0f;
        return;
    }

    if (max == r)
        *h = 60.0f * fmodf((g - b) / delta, 6.0f);
    else if (max == g)
        *h = 60.0f * (((b - r) / delta) + 2.0f);
    else
        *h = 60.0f * (((r - g) / delta) + 4.0f);

    if (*h < 0.0f)
        *h += 360.0f;
}

static inline void hsv_to_rgb(
    float h, float s, float v,
    float *r, float *g, float *b)
{
    if (s == 0.0f)
    {
        *r = v;
        *g = v;
        *b = v;
        return;
    }

    while (h < 0.0f)   h += 360.0f;
    while (h >= 360.0f) h -= 360.0f;

    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float rr, gg, bb;

    if (h < 60.0f)
    {
        rr = c; gg = x; bb = 0;
    }
    else if (h < 120.0f)
    {
        rr = x; gg = c; bb = 0;
    }
    else if (h < 180.0f)
    {
        rr = 0; gg = c; bb = x;
    }
    else if (h < 240.0f)
    {
        rr = 0; gg = x; bb = c;
    }
    else if (h < 300.0f)
    {
        rr = x; gg = 0; bb = c;
    }
    else
    {
        rr = c; gg = 0; bb = x;
    }

    *r = rr + m;
    *g = gg + m;
    *b = bb + m;
}


const char* g_windowTitle = "Body Scanner";

void mouse_pos_snap_fix();


void render_init(RenderContext* render);
void render_update(RenderContext* render);
void render_cleanup(RenderContext* render);

Buffer make_buffer(BScanContext* context);
FloatBuffer make_float_buffer(BScanContext* context);
Point* make_point(BScanContext* context, int x, int y);
Edge* make_edge(BScanContext* context, Point* a, Point* b);

#define ENCODE_RGB(W,R,G,B) W = ((word)(R)) | ((word)(G) << 8) | ((word)(B) << 16) | 0xFF000000;
#define DECODE_RGB(W,R,G,B) ( R = ((W)) & 0xFF, G = ((W) >> 8) & 0xFF, B = ((W) >> 16) & 0xFF )

#define BRIGHTNESS (0.2126*R + 0.7152*G + 0.0722*B)
#define BRIGHTNESS2(R,G,B) (0.2126*R + 0.7152*G + 0.0722*B)


void yuv_to_rgb(Buffer dst_buf, Buffer src_buf) {
    if (!src_buf.pixels)
        return;

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


void yuv_to_gray(Buffer dst_buf, Buffer src_buf) {
    if (!src_buf.pixels)
        return;

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

            int R0 = Y0; // + 1.402 * (V - 128);
            int G0 = Y0; // - 0.344 * (U - 128) - 0.714 * (V - 128);
            int B0 = Y0; // + 1.772 * (U - 128);
            int R1 = Y1; // + 1.402 * (V - 128);
            int G1 = Y1; // - 0.344 * (U - 128) - 0.714 * (V - 128);
            int B1 = Y1; // + 1.772 * (U - 128);
            // R0 = CLAMP(R0);
            // G0 = CLAMP(G0);
            // B0 = CLAMP(B0);
            // R1 = CLAMP(R1);
            // G1 = CLAMP(G1);
            // B1 = CLAMP(B1);
            
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


void sobel(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;


    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B,L;
            int gx = 0;
            int gy = 0;

            #define SOBEL_SUM(SX,SY,K0,K1) \
                W = src[(x + SX) + (y+SY) * pw]; \
                DECODE_RGB(W, R, G, B); \
                L = BRIGHTNESS; \
                gx += L*K0; \
                gy += L*K1;


            SOBEL_SUM(-1, -1, -1, -1)
            SOBEL_SUM( 0, -1, -2, 0)
            SOBEL_SUM( 1, -1, -1, 1)

            SOBEL_SUM(-1, 0, 0, -2)
            SOBEL_SUM( 0, 0, 0, 0)
            SOBEL_SUM( 1, 0, 0, 2)

            SOBEL_SUM(-1, 1, 1, -1)
            SOBEL_SUM( 0, 1, 2, 0)
            SOBEL_SUM( 1, 1, 1, 1)


            int value = ABS(gx) + ABS(gy);
            // int value = sqrt(gx*gx + gy*gy);

            dst[x + y * pw] = value;
        }
    }
}

void sobel_rgb(Buffer dst_buf, Buffer src_buf) {

    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;


    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word sW;
            word W;
            int R,G,B,L;
            int gx = 0;
            int gy = 0;

            #define SOBEL_SUM(SX,SY,K0,K1) \
                W = src[(x + SX) + (y+SY) * pw]; \
                DECODE_RGB(W, R, G, B); \
                L = BRIGHTNESS; \
                gx += L*K0; \
                gy += L*K1;


            SOBEL_SUM(-1, -1, -1, -1)
            SOBEL_SUM( 0, -1, -2, 0)
            SOBEL_SUM( 1, -1, -1, 1)

            SOBEL_SUM(-1, 0, 0, -2)
            SOBEL_SUM( 0, 0, 0, 0)
            sW = W;
            SOBEL_SUM( 1, 0, 0, 2)

            SOBEL_SUM(-1, 1, 1, -1)
            SOBEL_SUM( 0, 1, 2, 0)
            SOBEL_SUM( 1, 1, 1, 1)


            int value = ABS(gx) + ABS(gy);
            // int value = sqrt(gx*gx + gy*gy);
            if (value > 128) {
                value = 0xFF;
            } else {
                value = sW;
            }

            dst[x + y * pw] = value;
        }
    }
}


void subtract(Buffer dst_buf, FloatBuffer src_buf) {
    word* dst = dst_buf.pixels;
    FloatPixel* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B;
            float sR, sG, sB;

            FloatPixel* fp = &src[x + y * pw];
            sR = fp->r;
            sG = fp->g;
            sB = fp->b;

            W = dst[x + y * pw];
            DECODE_RGB(W, R, G, B);

            int dr = ABS(R - sR) * 2; 
            int dg = ABS(G - sG) * 3;
            int db = ABS(B - sB) * 1;
            const int diffVal = 13;
            if (dr < diffVal || dg < diffVal || db < diffVal) {
                R = 0;
                G = 0;
                B = 0;
            }

            ENCODE_RGB(W, R, G, B);
            dst[x + y * pw] = W;

        }
    }
}

void dim_subtract(Buffer dst_buf, FloatBuffer src_buf) {
    word* dst = dst_buf.pixels;
    FloatPixel* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B;
            float sR, sG, sB;

            FloatPixel* fp = &src[x + y * pw];
            sR = fp->r;
            sG = fp->g;
            sB = fp->b;
            // DECODE_RGB(W, sR, sG, sB);

            W = dst[x + y * pw];
            DECODE_RGB(W, R, G, B);


            // int diff = ABS(sR - R) + ABS(sG - G) + ABS(sB - B);
            fp->r = CLAMP(sR * 0.9 + R * 0.1);
            fp->g = CLAMP(sG * 0.9 + G * 0.1);
            fp->b = CLAMP(sB * 0.9 + B * 0.1);
            // if (diff > 260) {
            //     fp->r = 0.0f;
            //     fp->g = 0.0f;
            //     fp->b = 0.0f;
            // }

            // R = diff;
            // G = 0;
            // B = 0;

            R = CLAMP(R - sR);
            G = CLAMP(G - sG);
            B = CLAMP(B - sB);

            // R = CLAMP(MAX(ABS(R-sR), R * 0.95f));
            // G = CLAMP(MAX(ABS(G-sG), G * 0.95f));
            // B = CLAMP(MAX(ABS(B-sB), B * 0.95f));

            ENCODE_RGB(W, R, G, B);
            dst[x + y * pw] = W;

        }
    }
}


void update_background(FloatBuffer dst_buf, Buffer src_buf) {
    FloatPixel* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B;

            W = src[x + y * pw];
            DECODE_RGB(W, R, G, B);

            FloatPixel* fp = &dst[x + y * pw];
            fp->r = R;
            fp->g = G;
            fp->b = B;
        }
    }
}


void saturate(Buffer dst_buf, Buffer src_buf) {
    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    float saturateFactor = 2.0f;
    // float contrast = 1.f;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int R,G,B;

            W = src[x + y * pw];
            DECODE_RGB(W, R, G, B);

            // R = CLAMP((R - 128) * contrast + 128);
            // G = CLAMP((G - 128) * contrast + 128);
            // B = CLAMP((B - 128) * contrast + 128);
            float r = R / 255.0f;
            float g = G / 255.0f;
            float b = B / 255.0f;

            float h, s, v;
            rgb_to_hsv(r, g, b, &h, &s, &v);

            // Increase saturation
            s *= saturateFactor;
            if (s > 1.0f)
                s = 1.0f;

            hsv_to_rgb(h, s, v, &r, &g, &b);

            R = (int)(r * 255.0f + 0.5f);
            G = (int)(g * 255.0f + 0.5f);
            B = (int)(b * 255.0f + 0.5f);

            ENCODE_RGB(W, R, G, B);

            dst[x + y * pw] = W;
        }
    }
}


void flatten(Buffer dst_buf, Buffer src_buf) {
    word* dst = dst_buf.pixels;
    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    for (int y = 0; y < ph; y++) {
        for (int x = 0; x < pw; x++) {
            word W;
            int sR = 0, sG = 0, sB = 0;
            int R,G,B;

            #define FLATTEN_AVG(SX,SY) \
                W = src[(x + SX) + (y+SY) * pw]; \
                DECODE_RGB(W, R, G, B); \
                sR += R; \
                sG += G; \
                sB += B; 

            FLATTEN_AVG(-1, -1)
            FLATTEN_AVG( 0, -1)
            FLATTEN_AVG( 1, -1)

            FLATTEN_AVG(-1, 0)
            FLATTEN_AVG( 0, 0)
            FLATTEN_AVG( 1, 0)

            FLATTEN_AVG(-1, 1)
            FLATTEN_AVG( 0, 1)
            FLATTEN_AVG( 1, 1)

            int weight = 9;

            ENCODE_RGB(W, sR/weight, sG/weight, sB/weight);

            dst[x + y * pw] = W;
        }
    }
}


void point_sum(BScanContext* context, Buffer src_buf) {

    word* src = src_buf.pixels;
    int pw = src_buf.w;
    int ph = src_buf.h;

    Graph* graph = context->graph;
    int screenEdgePadding = 5;

    const int threshold = 6;

    for (int y = screenEdgePadding; y < ph - 2*screenEdgePadding; y++) {
        for (int x = screenEdgePadding; x < pw - 2*screenEdgePadding; x++) {
            word W;
            int R,G,B;
            int strength;
            W = src[x + y * pw];
            DECODE_RGB(W, R,G,B);
            strength = R;
            // strength = BRIGHTNESS;

            if (strength < 33)
                continue;

            
            bool tooClose = false;
            for (int i=0;i<graph->points_len;i++) {
                Point* b = &graph->points[i];

                int dx = x - b->x;
                int dy = y - b->y;

                int dist = dx*dx + dy*dy;
                if (dist < threshold*threshold) {
                    tooClose = true;
                    break;
                }
            }
            
            if (!tooClose) {
                // Point* dst = &graph->points[newPointLen];
                // *dst = *a;
                // newPointLen++;
                make_point(context, x, y);
            }
        }
    }
}

// Average very bad
// Vector2 detect_center(BScanContext* context) {
//     Graph* graph = context->graph;

//     int sumX = 0;
//     int sumY = 0;

//     for (int pi=0;pi<graph->points_len;pi++) {
//         Point* a = &graph->points[pi];

//         sumX += a->x;
//         sumY += a->y;
//     }

//     Vector2 center = {
//         .x = sumX / graph->points_len,
//         .y = sumY / graph->points_len,
//     };
//     return center;
// }

static int compare_int(const void* a, const void* b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

Vector2 detect_center(BScanContext* context) {
    Graph* graph = context->graph;

    if (graph->points_len == 0)
        return (Vector2){0, 0};

    int count = graph->points_len;

    // We may make a huge stack here
    int xs[count];
    int ys[count];

    for (int i = 0; i < count; i++) {
        xs[i] = graph->points[i].x;
        ys[i] = graph->points[i].y;
    }

    qsort(xs, count, sizeof(int), compare_int);
    qsort(ys, count, sizeof(int), compare_int);

    Vector2 center;

    if (count & 1) {
        // Odd number of points
        center.x = xs[count / 2];
        center.y = ys[count / 2];
    } else {
        // Even number of points
        center.x = (xs[count / 2 - 1] + xs[count / 2]) / 2.0f;
        center.y = (ys[count / 2 - 1] + ys[count / 2]) / 2.0f;
    }

    return center;
}


Vector2 detect_head(BScanContext* context, Vector2 center) {
    Graph* graph = context->graph;

    if (graph->points_len == 0)
        return (Vector2){0, 0};

    int count = graph->points_len;

    int xs[count];
    int ys[count];

    int finalLength = 0;

    const int neckHeight = 60; // If person is far away then this needs to be smaller.
    for (int i = 0; i < count; i++) {
        if (graph->points[i].y > center.y - neckHeight) {
            continue;
        }
        xs[finalLength] = graph->points[i].x;
        ys[finalLength] = graph->points[i].y;
        finalLength++;
    }

    qsort(xs, finalLength, sizeof(int), compare_int);
    qsort(ys, finalLength, sizeof(int), compare_int);

    Vector2 output;

    if (finalLength & 1) {
        // Odd number of points
        output.x = xs[finalLength / 2];
        output.y = ys[finalLength / 2];
    } else {
        // Even number of points
        output.x = (xs[finalLength / 2 - 1] + xs[finalLength / 2]) / 2.0f;
        output.y = (ys[finalLength / 2 - 1] + ys[finalLength / 2]) / 2.0f;
    }

    return output;
}


float scatter_score(BScanContext* context, Vector2 center) {
    Graph* graph = context->graph;

    int variance = 0;

    for (int pi=0;pi<graph->points_len;pi++) {
        Point* a = &graph->points[pi];
        int dx = center.x - a->x;
        int dy = center.y - a->y;
        variance += dx*dx + dy*dy;
    }

    if (graph->points_len <= 0)
        return 0;
    return variance / graph->points_len;
}

void edge_sum(BScanContext* context) {

    Graph* graph = context->graph;

    const int threshold = 5;

    for (int pi=0;pi<graph->points_len;pi++) {
        Point* a = &graph->points[pi];

        #define MAX_DIST 999
        #define MAX_POINTS 1

        Point* closest[MAX_POINTS]  = { 0 };
        int closestDist[MAX_POINTS];
        for (int ci=0;ci<ARRAY_LENGTH(closest);ci++)
            closestDist[ci] = MAX_DIST;

        for (int i=pi+1;i<graph->points_len;i++) {
            Point* b = &graph->points[i];

            int dx = a->x - b->x;
            int dy = a->y - b->y;
            int dist = dx*dx + dy*dy;

            for (int ci=0;ci<ARRAY_LENGTH(closest);ci++) {
                if (dist < closestDist[ci]) {
                    closestDist[ci] = dist;
                    closest[ci] = b;
                    break;
                }
            }
        }

        for (int ci=0;ci<ARRAY_LENGTH(closest);ci++) {
            if (!closest[ci])
                break;
            make_edge(context, a, closest[ci]);
        }
    }
}




void prepare_skeleton(Skeleton* skel) {
    
    skel->bones_len = BONE_COUNT;


    // Center bones

    skel->bones[BONE_TORSO] = (Bone){
        .parent = 0,
        .localPos = (Vector3){0, 0.0f, 0},
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
            bone->worldPos.y += 0.9f;
            continue;
        }

        Vector3 offset = Vector3RotateByQuaternion(bone->localPos, parent->worldRot);
        bone->worldPos = Vector3Add(parent->worldPos, offset);
        bone->worldRot = QuaternionMultiply(parent->worldRot, bone->localRot);

        if (i == BONE_HEAD) {
            bone->worldRot = QuaternionIdentity();
            bone->worldPos = bone->localPos;
            bone->worldPos.y += 0.9f;
        }
    }
}

typedef enum {
    MODE_ALL_FILTERS,
    MODE_RAW_VIDEO,
    MODE_ONLY_SUBTRACT,
} FilterMode;

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
    graph->points_cap = 5000;
    graph->points = malloc(graph->points_cap * sizeof(Point));


    // Camera raycam = { { 0.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    Camera raycam = { 0 };
    raycam.position = (Vector3){ 0.0f, 1.4f, 7.0f };    // Camera position
    raycam.target = (Vector3){ 0.0, 0.5f, 0 };    // Camera looking at point
    raycam.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    raycam.fovy = 45.0f;                                // Camera field-of-view Y
    raycam.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    DisableCursor();                // Limit cursor to relative movement inside the window

    FloatBuffer backgroundImage = make_float_buffer(context);

    Vector2 previousCenter = {0};
    Vector2 previousHead = {0};

    bool hasBackground = false;

    FilterMode filterMode = MODE_ALL_FILTERS;

    while (!WindowShouldClose()) {
        
        camera_update(context->camera);


        Buffer tmp;
        Buffer src = temp0;
        Buffer dst = temp1;

        if (IsKeyPressed(KEY_ONE)) {
            filterMode = MODE_ALL_FILTERS;
        } else if (IsKeyPressed(KEY_TWO)) {
            filterMode = MODE_RAW_VIDEO;
        } else if (IsKeyPressed(KEY_THREE)) {
            filterMode = MODE_ONLY_SUBTRACT;
        }

        #define PIPE_START(F, INPUT) F(dst, INPUT); tmp = src; src = dst; dst = tmp;
        #define PIPE(F) F(dst, src); tmp = src; src = dst; dst = tmp;

        PIPE_START(yuv_to_rgb, camera->output)
        // PIPE_START(yuv_to_gray, camera->output)
        // PIPE(grayscale)
        // PIPE(blur)
        PIPE(gaus3x3)
        // PIPE(saturate)
        // PIPE(gaus5x5)

        if ((!hasBackground || IsKeyPressed(KEY_R)) && camera->output.pixels) {
            update_background(backgroundImage, src);

            if (!hasBackground) {
                hasBackground = true;
                continue;
            }
        }

        if (filterMode == MODE_ONLY_SUBTRACT || filterMode == MODE_ALL_FILTERS) {
            subtract(src, backgroundImage);
        // dim_subtract(src, backgroundImage);
        }

        // PIPE(flatten)

        if (filterMode == MODE_ALL_FILTERS) {
            // PIPE(edge)
            PIPE(sobel)
            // PIPE(sobel_rgb)
        }




        graph->points_len = 0;
        graph->edges_len = 0;

        if (filterMode == MODE_ALL_FILTERS) {
            point_sum(context, src);
        }

        float variance = scatter_score(context, previousCenter);
        float varianceThreshold = 23.f;
        Vector2 center;
        Vector2 headCenter;
        if (variance < varianceThreshold * varianceThreshold || graph->points_len > 70) {
            center = detect_center(context);

            center.x = center.x * 0.2f + previousCenter.x * 0.8f;
            center.y = center.y * 0.2f + previousCenter.y * 0.8f;
            previousCenter = center;

            headCenter = detect_head(context, center);

            headCenter.x = headCenter.x * 0.2f + previousHead.x * 0.8f;
            headCenter.y = headCenter.y * 0.2f + previousHead.y * 0.8f;
            previousHead = headCenter;

        } else {
            printf("LOW %f\n", sqrt(variance));
            center = previousCenter;
            headCenter = previousHead;
        }



        Bone* torso = &skeleton->bones[BONE_TORSO];
        torso->localPos.x = 2 * center.x / render->target.w - 1;
        torso->localPos.y = 1 - 2 * center.y / render->target.h;

        Bone* head = &skeleton->bones[BONE_HEAD];
        head->localPos.x = 2 * (headCenter.x) / render->target.w - 1;
        head->localPos.y = 1 - 2 * (headCenter.y) / render->target.h;

        // edge_sum(context);

        // printf("%d\n", graph->points_len);

        render->target = src;

        mouse_pos_snap_fix();

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


        // Apply motion to skeleton bones.

        // Let's first apply motion to one bone.
        // Let's get any kind of motion going one at all.
        // Torso, head, arm, leg.

        // ARm is moving around the most.

        // everything spans from the torso

        // head is the highest point
        // we don't want to assume this, person could be lying down.
        // Do we want to track that? it would be cool and nice with few restrictions.


        // Calculate bone world position from relative positions
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
    InitWindow(render->screenWidth, render->screenHeight, g_windowTitle);

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
FloatBuffer make_float_buffer(BScanContext* context) {
    RenderContext* render = context->render;
    int w = context->camera->output.w;
    int h = context->camera->output.h;
    int extra_padding = 2;
    FloatBuffer buffer = { .w = w, .h = h };
    buffer.pixels = malloc(w * (h + 2*extra_padding) * FLOAT_PIXEL_SIZE);
    buffer.pixels += extra_padding * w + 20;
    return buffer;
}


Point* make_point(BScanContext* context, int x, int y) {
    if (context->graph->points_len >= context->graph->points_cap) {
        return NULL;
    }
    // ASSERT(context->graph->points_len < context->graph->points_cap);
    Point* p = &context->graph->points[context->graph->points_len++];
    p->x = x;
    p->y = y;
    return p;
}
Edge* make_edge(BScanContext* context, Point* a, Point* b) {
    if (context->graph->edges_len >= context->graph->edges_cap) {
        return NULL;
    }
    ASSERT(context->graph->edges_len < context->graph->edges_cap);
    Edge* edge = &context->graph->edges[context->graph->edges_len++];
    edge->a = a;
    edge->b = b;
    return edge;
}



/*
    Raylib stuff
*/


// typedef struct { int x; int y; } Point;
typedef struct { unsigned int width; unsigned int height; } Size;


#define MAX_KEYBOARD_KEYS            512        // Maximum number of keyboard keys supported
#define MAX_MOUSE_BUTTONS              8        // Maximum number of mouse buttons supported
#define MAX_GAMEPADS                   4        // Maximum number of gamepads supported

#define MAX_GAMEPAD_NAME_LENGTH      128        // Maximum number of characters in a gamepad name (byte size)
#define MAX_GAMEPAD_AXES               8        // Maximum number of axes supported (per gamepad)
#define MAX_GAMEPAD_BUTTONS           32        // Maximum number of buttons supported (per gamepad)
#define MAX_GAMEPAD_VIBRATION_TIME     2.0f     // Maximum vibration time in seconds
#define MAX_TOUCH_POINTS               8        // Maximum number of touch points supported
#define MAX_KEY_PRESSED_QUEUE         16        // Maximum number of keys in the key input queue
#define MAX_CHAR_PRESSED_QUEUE        16        // Maximum number of characters in the char input queue


// Core global state context data
typedef struct CoreData {
    struct {
        const char *title;                  // Window text title const pointer
        unsigned int flags;                 // Configuration flags (bit based), keeps window state
        bool ready;                         // Check if window has been initialized successfully
        bool shouldClose;                   // Check if window set for closing
        bool resizedLastFrame;              // Check if window has been resized last frame
        bool eventWaiting;                  // Wait for events before ending frame
        bool usingFbo;                      // Using FBO (RenderTexture) for rendering instead of default framebuffer

        Size display;                       // Display width and height (monitor, device-screen, LCD, ...)
        Size screen;                        // Screen current width and height
        Point position;                     // Window current position
        Size previousScreen;                // Screen previous width and height (required on fullscreen/borderless-windowed toggle)
        Point previousPosition;             // Window previous position (required on fullscreen/borderless-windowed toggle)
        Size render;                        // Screen framebuffer width and height
        Point renderOffset;                 // Screen framebuffer render offset (Not required anymore?)
        Size currentFbo;                    // Current framebuffer render width and height (depends on active render texture)
        Size screenMin;                     // Screen minimum width and height (for resizable window)
        Size screenMax;                     // Screen maximum width and height (for resizable window)
        Matrix screenScale;                 // Matrix to scale screen (framebuffer rendering)

        char **dropFilepaths;               // Store dropped files paths pointers (provided by GLFW)
        unsigned int dropFileCount;         // Count dropped files strings

    } Window;
    struct {
        const char *basePath;               // Base path for data storage

    } Storage;
    struct {
        struct {
            int exitKey;                    // Default exit key
            char currentKeyState[MAX_KEYBOARD_KEYS]; // Registers current frame key state
            char previousKeyState[MAX_KEYBOARD_KEYS]; // Registers previous frame key state

            // NOTE: Since key press logic involves comparing previous vs current key state,
            // key repeats needs to be handled specially
            char keyRepeatInFrame[MAX_KEYBOARD_KEYS]; // Registers key repeats for current frame

            int keyPressedQueue[MAX_KEY_PRESSED_QUEUE]; // Input keys queue
            int keyPressedQueueCount;       // Input keys queue count

            int charPressedQueue[MAX_CHAR_PRESSED_QUEUE]; // Input characters queue (unicode)
            int charPressedQueueCount;      // Input characters queue count

        } Keyboard;
        struct {
            Vector2 offset;                 // Mouse offset
            Vector2 scale;                  // Mouse scaling
            Vector2 currentPosition;        // Mouse position on screen
            Vector2 previousPosition;       // Previous mouse position
            Vector2 lockedPosition;         // Mouse position when locked

            int cursor;                     // Tracks current mouse cursor
            bool cursorHidden;              // Track if cursor is hidden
            bool cursorLocked;              // Track if cursor is locked (disabled)
            bool cursorOnScreen;            // Tracks if cursor is inside client area

            char currentButtonState[MAX_MOUSE_BUTTONS]; // Registers current mouse button state
            char previousButtonState[MAX_MOUSE_BUTTONS]; // Registers previous mouse button state
            Vector2 currentWheelMove;       // Registers current mouse wheel variation
            Vector2 previousWheelMove;      // Registers previous mouse wheel variation

        } Mouse;
        struct {
            int pointCount;                                 // Number of touch points active
            int pointId[MAX_TOUCH_POINTS];                  // Point identifiers
            Vector2 position[MAX_TOUCH_POINTS];             // Touch position on screen
            Vector2 previousPosition[MAX_TOUCH_POINTS];     // Previous touch position on screen
            char currentTouchState[MAX_TOUCH_POINTS];       // Registers current touch state
            char previousTouchState[MAX_TOUCH_POINTS];      // Registers previous touch state

        } Touch;
        struct {
            int lastButtonPressed;          // Register last gamepad button pressed
            int axisCount[MAX_GAMEPADS];    // Register number of available gamepad axes
            bool ready[MAX_GAMEPADS];       // Flag to know if gamepad is ready
            char name[MAX_GAMEPADS][MAX_GAMEPAD_NAME_LENGTH];               // Gamepad name holder
            char currentButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS];     // Current gamepad buttons state
            char previousButtonState[MAX_GAMEPADS][MAX_GAMEPAD_BUTTONS];    // Previous gamepad buttons state
            float axisState[MAX_GAMEPADS][MAX_GAMEPAD_AXES];                // Gamepad axes state

        } Gamepad;
    } Input;
    struct {
        double current;                     // Current time measure (seconds)
        double previous;                    // Previous time measure (seconds)
        double update;                      // Time measure for frame update (seconds)
        double draw;                        // Time measure for frame draw (seconds)
        double frame;                       // Time measure for one frame (seconds)
        double target;                      // Desired time for one frame, if 0 not applied (seconds)
        unsigned long long base;            // Base time measure for hi-res timer (ticks or nanoseconds)
        unsigned int frameCounter;          // Frame counter (frames)

    } Time;
} CoreData;


CoreData* RaylibCore() {
    // This happens to work on my system.

    uint64_t address   = (uint64_t)&IsWindowReady;
    uint32_t offset = *(uint32_t*)(address + 3) + 7 - offsetof(CoreData, Window.ready);
    CoreData* CORE = (CoreData*)(address + offset);
    // From GDB:
    //     IsWindowReady:
    //    0x00007ffff7cdcb80 <+0>:	movzbl 0x11ab45(%rip),%eax   # 0x7ffff7df76cc <CORE+12>
    //    0x00007ffff7cdcb87 <+7>:	ret

    // Verify that we got the right address.
    if (CORE->Window.title != g_windowTitle)
        return NULL;

    // Verify that the struct is correct. It's actually wrong so
    // we don't check it and return it anyway.
    // if ((float)CORE->Time.frame != GetFrameTime()) {
    //     printf("FRAME DIFF %f %f\n", (float)CORE->Time.frame, GetFrameTime());
    //     return NULL;
    // }
    return CORE;
}

void mouse_pos_snap_fix() {
    CoreData* CORE = RaylibCore();
    if (!CORE)  return;

    Vector2 delta = GetMouseDelta();
    // Prevent large snap, happens at start of program when we first move mouse.
    if (Vector2Length(delta) > 200) {
        CORE->Input.Mouse.previousPosition.x = CORE->Input.Mouse.currentPosition.x;
        CORE->Input.Mouse.previousPosition.y = CORE->Input.Mouse.currentPosition.y;
    }
}