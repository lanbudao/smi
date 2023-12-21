extern "C" {
#include <libavformat/avformat.h>
}
int main(int argc, char* argv[])
{
    printf("avformat version %d.\n", avformat_version());
    return 0;
}
