extern "C" {
#include <libavdevice/avdevice.h>
}
int main(int argc, char* argv[])
{
    printf("avdevice version %d.\n", avdevice_version());
    return 0;
}
