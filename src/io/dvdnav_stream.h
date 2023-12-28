#pragma once

extern "C" {
#include "dvdread/dvd_reader.h"
#include "dvdread/ifo_types.h"
#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"
#include "dvdnav/dvd_types.h"
#include "dvdnav/dvdnav.h"
}

#include "global.h"
NAMESPACE_BEGIN

/* state flags */
typedef enum {
    NAV_FLAG_EOF = 1 << 0,  /* end of stream has been reached */
    NAV_FLAG_WAIT = 1 << 1,  /* wait event */
    NAV_FLAG_WAIT_SKIP = 1 << 2,  /* wait skip disable */
    NAV_FLAG_CELL_CHANGE = 1 << 3,  /* cell change event */
    NAV_FLAG_WAIT_READ_AUTO = 1 << 4,  /* wait read auto mode */
    NAV_FLAG_WAIT_READ = 1 << 5,  /* suspend read from stream */
    NAV_FLAG_VTS_DOMAIN = 1 << 6,  /* vts domain */
    NAV_FLAG_SPU_SET = 1 << 7,  /* spu_clut is valid */
    NAV_FLAG_STREAM_CHANGE = 1 << 8,  /* title, chapter, audio or SPU */
    NAV_FLAG_AUDIO_CHANGE = 1 << 9,  /* audio stream change event */
    NAV_FLAG_SPU_CHANGE = 1 << 10, /* spu stream change event */
} dvdnav_state_t;

typedef struct {
    dvdnav_t *       dvdnav;              /* handle to libdvdnav stuff */
    unsigned int     duration;            /* in milliseconds */
    int              mousex, mousey;
    int              title;
    unsigned int     spu_clut[16];
    dvdnav_highlight_event_t hlev;
    int              still_length;        /* still frame duration */
    unsigned int     state;
} dvdnav_priv_t;

int fill_buffer(char *but, int len);

NAMESPACE_END