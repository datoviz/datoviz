#include <stdio.h>

#include "datoviz/datoviz.h"

int main(void)
{
    const char* version = dvz_version();
    if (version == NULL)
    {
        fprintf(stderr, "dvz_version() returned NULL\n");
        return 1;
    }
    printf("Datoviz version: %s\n", version);
    return 0;
}
