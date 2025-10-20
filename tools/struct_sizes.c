#include <stddef.h>
#include <stdio.h>

#include "_types.h"



int main(void);

int main(void)
{
    printf("{\n");
    printf("  \"DvzShape\": %u,\n", (unsigned)sizeof(DvzShape));
    printf("  \"DvzMVP\": %u,\n", (unsigned)sizeof(DvzMVP));
    printf("  \"DvzGuiEvent\": %u,\n", (unsigned)sizeof(DvzGuiEvent));
    printf("  \"DvzKeyboardEvent\": %u,\n", (unsigned)sizeof(DvzKeyboardEvent));
    printf("  \"DvzMouseWheelEvent\": %u,\n", (unsigned)sizeof(DvzMouseWheelEvent));
    printf("  \"DvzMouseDragEvent\": %u,\n", (unsigned)sizeof(DvzMouseDragEvent));
    printf("  \"DvzMouseEventUnion\": %u,\n", (unsigned)sizeof(DvzMouseEventUnion));
    printf("  \"DvzMouseEvent\": %u,\n", (unsigned)sizeof(DvzMouseEvent));
    printf("  \"DvzWindowEvent\": %u,\n", (unsigned)sizeof(DvzWindowEvent));
    printf("  \"DvzFrameEvent\": %u,\n", (unsigned)sizeof(DvzFrameEvent));
    printf("  \"DvzTimerEvent\": %u,\n", (unsigned)sizeof(DvzTimerEvent));
    printf("  \"DvzRequestsEvent\": %u,\n", (unsigned)sizeof(DvzRequestsEvent));
    printf("  \"DvzRecorderCommand\": %u,\n", (unsigned)sizeof(DvzRecorderCommand));
    printf("  \"DvzRequestContent\": %u,\n", (unsigned)sizeof(DvzRequestContent));
    printf("  \"DvzRequest\": %u\n", (unsigned)sizeof(DvzRequest));
    printf("}\n");
    return 0;
}
