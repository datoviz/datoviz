#include <stdio.h>

#include <datoviz.h>

int main(void)
{
    const char* version = dvz_version();
    if (version == NULL)
        return 1;

    printf("Datoviz %s\n", version);
    return 0;
}
