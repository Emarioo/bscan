#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef uint8_t  byte;
typedef uint32_t word;
#define PIXEL_SIZE 4
#define FLOAT_PIXEL_SIZE 12


typedef struct {
    word* pixels;
    int   w;
    int   h;
} Buffer;

typedef struct {
    float r,g,b;
} FloatPixel;

typedef struct {
    FloatPixel* pixels;
    int   w;
    int   h;
} FloatBuffer;
