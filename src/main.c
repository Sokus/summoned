#include <stdio.h>

#include "config.h"

int main(int argc, char *argv[])
{
    printf("Project version: %d.%d.%d\n",
           PROJECT_VERSION_MAJOR,
           PROJECT_VERSION_MINOR,
           PROJECT_VERSION_PATCH);

    return 0;
}