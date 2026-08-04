#pragma once

#include <raylib.h>
#include <raymath.h>

#include "bscan/buffer.h"

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

typedef enum {
    BONE_TORSO,
    BONE_HEAD,

    BONE_LEFT_SHOULDER,
    BONE_LEFT_ARM,
    BONE_LEFT_WRIST,

    BONE_LEFT_HIP,
    BONE_LEFT_KNEE,
    BONE_LEFT_ANKLE,

    BONE_RIGHT_SHOULDER,
    BONE_RIGHT_ELBOW,
    BONE_RIGHT_WRIST,

    BONE_RIGHT_HIP,
    BONE_RIGHT_KNEE,
    BONE_RIGHT_ANKLE,

    BONE_COUNT,
} BoneKind;

typedef struct {
    Vector3    localPos;
    Quaternion localRot;

    Vector3    worldPos;
    Quaternion worldRot;

    int        parent;
} Bone;


typedef struct {
    Bone bones[BONE_COUNT];
    int  bones_len;
} Skeleton;
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




