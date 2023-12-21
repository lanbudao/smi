extern "C" {
#include <libswscale/swscale.h>
}
int main(int argc, char* argv[])
{
    printf("swscale version %d.\n", swscale_version());
    return 0;
}
