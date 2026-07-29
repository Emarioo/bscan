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


typedef struct {
    word* pixels;
    int   w;
    int   h;
} Buffer;

