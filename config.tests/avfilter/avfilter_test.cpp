extern "C" {
#include <libavfilter/avfilter.h>
}
int main(int argc, char* argv[])
{
    printf("avfilter version %d.\n", avfilter_version());
    return 0;
}
