#pragma once

#include <raylib.h>
#include <raymath.h>

#include "bscan/buffer.h"
#include "bscan/common/types.h"

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point* a;
    Point* b;
} Edge;

typedef struct {
    Edge* edges;
    int   edges_len;
    int   edges_cap;

    Point* points;
    int    points_len;
    int    points_cap;
} Graph;


typedef struct {
    Vector3    localPos;
    Quaternion localRot;

    Vector3    worldPos;
    Quaternion worldRot;

    int        parent;
} Bone;


struct Skeleton {
    Bone bones[BONE_COUNT];
    int  bones_len;
};

typedef struct {
    Vector2 localPos;
    Vector2 worldPos;
    float   angle;
    float   length;
    int parent;
} ImageBone;

typedef struct {
    ImageBone bones[BONE_COUNT];
    int  bones_len;
} ImageSkeleton;




