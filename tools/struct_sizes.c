#include <stddef.h>
#include <stdio.h>

#include "datoviz_types.h"



int main(void);

int main(void)
{
    printf("{\n");
    printf("  \"DvzShape\": %zu,\n", sizeof(DvzShape));
    printf("  \"DvzMVP\": %zu,\n", sizeof(DvzMVP));
    printf("  \"DvzGuiEvent\": %zu,\n", sizeof(DvzGuiEvent));
    printf("  \"DvzKeyboardEvent\": %zu,\n", sizeof(DvzKeyboardEvent));
    printf("  \"DvzMouseButtonEvent\": %zu,\n", sizeof(DvzMouseButtonEvent));
    printf("  \"DvzMouseWheelEvent\": %zu,\n", sizeof(DvzMouseWheelEvent));
    printf("  \"DvzMouseDragEvent\": %zu,\n", sizeof(DvzMouseDragEvent));
    printf("  \"DvzMouseClickEvent\": %zu,\n", sizeof(DvzMouseClickEvent));
    printf("  \"DvzMouseEventUnion\": %zu,\n", sizeof(DvzMouseEventUnion));
    printf("  \"DvzMouseEvent\": %zu,\n", sizeof(DvzMouseEvent));
    printf("  \"DvzWindowEvent\": %zu,\n", sizeof(DvzWindowEvent));
    printf("  \"DvzFrameEvent\": %zu,\n", sizeof(DvzFrameEvent));
    printf("  \"DvzTimerEvent\": %zu,\n", sizeof(DvzTimerEvent));
    printf("  \"DvzRequestsEvent\": %zu\n", sizeof(DvzRequestsEvent));
    printf("}\n");
    return 0;
}
