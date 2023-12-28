#include "dvdnav_stream.h"
#include "utils/AVLog.h"

NAMESPACE_BEGIN
static int dvdnav_stream_read(dvdnav_priv_t * priv, unsigned char *buf, int *len)
{
    int event = DVDNAV_NOP;

    *len = -1;
    if (dvdnav_get_next_block(priv->dvdnav, buf, &event, len) != DVDNAV_STATUS_OK) {
        AVDebug("Error getting next block from DVD %d (%s)\n", event, dvdnav_err_to_string(priv->dvdnav));
        *len = -1;
    }
    else if (event != DVDNAV_BLOCK_OK && event != DVDNAV_NAV_PACKET) {
        *len = 0;
    }
    return event;
}



NAMESPACE_END