#include "../include/visky/constants.h"
#include <stdint.h>

static double VKY_CONSTANTS[VKY_MAX_CONSTANTS];
static bool VKY_CONSTANTS_INITIALIZED[VKY_MAX_CONSTANTS];

double vky_get_constant(VkyConstantName name, double default_value)
{
    return VKY_CONSTANTS_INITIALIZED[name] ? VKY_CONSTANTS[name] : default_value;
}

void vky_set_constant(VkyConstantName name, double value)
{
    VKY_CONSTANTS_INITIALIZED[name] = true;
    VKY_CONSTANTS[name] = value;
}

void vky_reset_constant(VkyConstantName name) {
    VKY_CONSTANTS_INITIALIZED[name] = false;
}

void vky_reset_all_constants() {
    memset(VKY_CONSTANTS_INITIALIZED, 0, VKY_MAX_CONSTANTS * sizeof(bool));
}
