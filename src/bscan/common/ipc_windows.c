
#include "bscan/common/types.h"


#include "Windows.h"


#define BSCAN_SHARED_MEMORY "Local\\bscan_tracker"


typedef struct {
    bool isServer;
    HANDLE handle;
    BScan_TrackerState* state;
} IPC_Context;


static IPC_Context g_bscan_ipc_context;

bool bscan_ipc_init() {
    if (g_bscan_ipc_context.state) {
        return true;
    }

    HANDLE handle = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(BScan_TrackerState),
        BSCAN_SHARED_MEMORY
    );
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    BScan_TrackerState* state = MapViewOfFile(
        handle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(BScan_TrackerState)
    );
    if (!state) {
        return false;
    }

    memset(state, 0, sizeof(*state));
    state->version = 1;

    g_bscan_ipc_context.isServer = true;
    g_bscan_ipc_context.handle   = handle;
    g_bscan_ipc_context.state    = state;
    return true;
}

bool bscan_ipc_connect() {
    if (g_bscan_ipc_context.state) {
        return true;
    }

    HANDLE handle = OpenFileMappingA(
        FILE_MAP_READ,
        FALSE,
        BSCAN_SHARED_MEMORY
    );
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    BScan_TrackerState* state =
        (BScan_TrackerState*)MapViewOfFile(
            handle,
            FILE_MAP_READ,
            0,
            0,
            sizeof(BScan_TrackerState)
        );
    if (!state) {
        return false;
    }

    g_bscan_ipc_context.handle = handle;
    g_bscan_ipc_context.state  = state;
    return true;
}

BScan_TrackerState* bscan_ipc_state() {
    return g_bscan_ipc_context.state;
}

bool bscan_fetch_bone(BScan_BoneKind kind, BScan_Bone* bone) {
    BScan_TrackerState* state = bscan_ipc_state();
    if (!state) {
        return false;
    }

    uint32_t a, b;

    do {
        a = state->sequence;

        *bone = state->bones[kind];

        b = state->sequence;
    } while (a != b || (a & 1)); // lock-free read

    return true;
}

