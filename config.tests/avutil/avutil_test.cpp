extern "C" {
#include <libavutil/avutil.h>
}
int main(int argc, char* argv[])
{
    printf("avutil info, %s.\n", av_version_info());
    return 0;
}
