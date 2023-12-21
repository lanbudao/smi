extern "C" {
#include <libswresample/swresample.h>
}
int main(int argc, char* argv[])
{
    printf("swresample version %d.\n", swresample_version());
    return 0;
}
