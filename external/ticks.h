#ifndef VKY_TICK_FORMAT_HEADER
#define VKY_TICK_FORMAT_HEADER

#include "../include/visky/common.h"
#include <stdint.h>
BEGIN_INCL_NO_WARN
#include <cglm/struct.h>
END_INCL_NO_WARN



#define VKY_AXES_NORMAL_RANGE(x)                                                                  \
    ((x) == 0 || ((VKY_AXES_DECIMAL_FORMAT_MIN <= (x)) && ((x) < VKY_AXES_DECIMAL_FORMAT_MAX)))



#define VKY_TICK_FORMAT_COUNT 2



typedef enum
{
    VKY_TICK_FORMAT_DECIMAL,
    VKY_TICK_FORMAT_SCIENTIFIC,
} VkyTickFormatType;



typedef struct VkyTickFormat VkyTickFormat;

typedef struct VkyAxesTickRange VkyAxesTickRange;

typedef struct VkyAxesContext VkyAxesContext;



struct VkyTickFormat
{
    VkyTickFormatType format_type;
    int32_t precision; // number of digits after the dot
    double legibility;
};



struct VkyAxesTickRange
{
    double vmin, vmax, step;
    double vmin_ndc, vmax_ndc, step_ndc;
    VkyTickFormat format;
};



struct VkyAxesContext
{
    uint8_t coord; // TODO enum
    vec2 glyph_size, viewport_size;
    double dpi_factor;
    bool debug;
};


#endif
