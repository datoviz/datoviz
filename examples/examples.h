/*************************************************************************************************/
/*  C examples included in the codebase and built into the CLI tool                              */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLES_HEADER
#define DVZ_EXAMPLES_HEADER

#define SCREENSHOT(name)                                                                          \
    dvz_app_run(app, 5);                                                                          \
    char path[1024];                                                                              \
    snprintf(path, sizeof(path), "%s/docs/images/screenshots/%s.png", ROOT_DIR, name);            \
    dvz_screenshot_file(canvas, path);



#include "../include/datoviz/common.h"
#include "custom_graphics.h"
#include "custom_visual.h"



#endif
