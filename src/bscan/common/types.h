#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>



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
} BScan_BoneKind;

typedef struct {
    float pos[3];
    float rot[4];
} BScan_Bone;

typedef struct {
    bool debugMode;
} BScan_Settings;

typedef struct {

    uint32_t version;
    uint32_t sequence;

    BScan_Settings settings;

    BScan_Bone bones[12];

} BScan_TrackerState;

typedef struct Skeleton Skeleton;



#ifdef __cplusplus
extern "C" {
#endif

bool bscan_ipc_init();

bool bscan_ipc_connect();

BScan_TrackerState* bscan_ipc_state();

bool bscan_update_bones(Skeleton* skeleton);
bool bscan_fetch_bone(BScan_BoneKind kind, BScan_Bone* bone);
bool bscan_get_settings(BScan_Settings* settings);
bool bscan_set_settings(BScan_Settings* settings);

#ifdef __cplusplus
} // extern "C"
#endif
