extern "C" {
#include <libavcodec/avcodec.h>
}
int main(int argc, char* argv[])
{
    printf("avcodec version %d.\n", avcodec_version());
    return 0;
}
