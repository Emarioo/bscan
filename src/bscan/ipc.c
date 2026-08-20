
#include "bscan/bscan.h"

bool bscan_update_bones(Skeleton* skeleton) {
    BScan_TrackerState* state = bscan_ipc_state();
    if (!state) {
        return false;
    }

    state->sequence++;

    for (int i=0;i<skeleton->bones_len;i++) {
        Bone*       src = &skeleton->bones[i];
        BScan_Bone* dst = &state->bones[i];

        memcpy(dst->pos, &src->worldPos, sizeof(dst->pos));
        memcpy(dst->rot, &src->worldRot, sizeof(dst->rot));
    }

    state->sequence++;
    return true;
}

bool bscan_set_settings(BScan_Settings* settings) {
    BScan_TrackerState* state = bscan_ipc_state();
    if (!state) {
        return false;
    }
    
    state->sequence++;

    state->settings = *settings;

    state->sequence++;

    return true;
}
