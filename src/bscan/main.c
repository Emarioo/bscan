
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bscan/bscan.h"


int ray_test(void);
int cam_test(void);

int main() {

    printf("I'm alive!\n");

    BScanContext* context = calloc(1, sizeof(BScanContext));

    bscan_loop(context);


    // cam_test();
    
    // ray_test();
    return 0;
}
