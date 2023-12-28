#ifndef DVDIO_P_H
#define DVDIO_P_H

extern "C" {
#include "dvdread/dvd_reader.h"
#include "dvdread/ifo_types.h"
#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"
#include "dvdnav/dvd_types.h"
#include "dvdnav/dvdnav.h"
}

#include "sdk/global.h"

NAMESPACE_BEGIN

#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#include <windows.h>
#endif

#define FFMin(a, b) (a < b) ? a : b
#define FFMax(a, b) (a > b) ? a : b
#define STREAM_BUFFER_MIN 2048
#define STREAM_BUFFER_SIZE (STREAM_BUFFER_MIN) // must be at least 2*STREAM_BUFFER_MIN
#define STREAM_MAX_SECTOR_SIZE (8*1024)
#define STREAM_REDIRECTED -2
#define STREAM_UNSUPPORTED -1
#define STREAM_ERROR 0
#define STREAM_OK    1
#define MAX_STREAM_PROTOCOLS 10

#define HAVE_WINSOCK2_H 1

#define STREAM_CTRL_RESET 0
#define STREAM_CTRL_GET_TIME_LENGTH 1
#define STREAM_CTRL_SEEK_TO_CHAPTER 2
#define STREAM_CTRL_GET_CURRENT_CHAPTER 3
#define STREAM_CTRL_GET_NUM_CHAPTERS 4
#define STREAM_CTRL_GET_CURRENT_TIME 5
#define STREAM_CTRL_SEEK_TO_TIME 6
#define STREAM_CTRL_GET_SIZE 7
#define STREAM_CTRL_GET_ASPECT_RATIO 8
#define STREAM_CTRL_GET_NUM_ANGLES 9
#define STREAM_CTRL_GET_ANGLE 10
#define STREAM_CTRL_SET_ANGLE 11
#define STREAM_CTRL_GET_NUM_TITLES 12
#define STREAM_CTRL_GET_LANG 13
#define STREAM_CTRL_GET_CURRENT_TITLE 14
#define STREAM_CTRL_GET_CURRENT_CHANNEL 15

/*Demuxer*/
#define DEMUXER_TYPE_UNKNOWN 0
#define DEMUXER_TYPE_MPEG_ES 1
#define DEMUXER_TYPE_MPEG_PS 2

/*Stream Type*/
#define STREAMTYPE_DVD  3      // libdvdread
#define STREAMTYPE_DVDNAV 9    // we cannot safely "seek" in this...

#define STREAM_READ  0
#define STREAM_WRITE 1

const char * const dvd_audio_stream_types[8] = { "ac3","unknown","mpeg1","mpeg2ext","lpcm","unknown","dts" };
const char * const dvd_audio_stream_channels[6] = { "mono", "stereo", "unknown", "unknown", "5.1/6.1", "5.1" };

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

static int dvdtimetomsec(dvd_time_t *dt)
{
    static int framerates[4] = { 0, 2500, 0, 2997 };
    int framerate = framerates[(dt->frame_u & 0xc0) >> 6];
    int msec = (((dt->hour & 0xf0) >> 3) * 5 + (dt->hour & 0x0f)) * 3600000;
    msec += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60000;
    msec += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f)) * 1000;
    if (framerate > 0)
        msec += (((dt->frame_u & 0x30) >> 3) * 5 + (dt->frame_u & 0x0f)) * 100000 / framerate;
    return msec;
}

static inline int dvdnav_get_duration(int length) {
    return (length == 255) ? 0 : length * 1000;
}

static int dvdnav_stream_read(dvdnav_priv_t * priv, unsigned char *buf, int *len) {
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

static void identify_chapters(dvdnav_t *nav, int title)
{
    uint64_t *parts = NULL, duration = 0;
    uint32_t n, i, t;
    n = dvdnav_describe_title_chapters(nav, title, &parts, &duration);
    if (parts) {
        t = duration / 90;
        AVDebug("ID_DVD_TITLE_%d_LENGTH=%d.%03d\n", title, t / 1000, t % 1000);
        AVDebug("ID_DVD_TITLE_%d_CHAPTERS=%d\n", title, n);
        AVDebug("TITLE %u, CHAPTERS: ", title);

        for (i = 0; i < n; i++) {
            t = parts[i] / 90000;
            AVDebug("%02d:%02d:%02d,", t / 3600, (t / 60) % 60, t % 60);
        }
        free(parts);
        AVDebug("\n");
    }
}

static void show_audio_subs_languages(dvdnav_t *nav)
{
    uint8_t lg;
    uint16_t i, lang, format, id, channels;
    int base[7] = { 128, 0, 0, 0, 160, 136, 0 };
    for (i = 0; i < 8; i++)
    {
        char tmp[] = "unknown";
        lg = dvdnav_get_audio_logical_stream(nav, i);
        if (lg == 0xff) continue;
        channels = dvdnav_audio_stream_channels(nav, lg);
        if (channels == 0xFFFF)
            channels = 2; //unknown
        else
            channels--;
        lang = dvdnav_audio_stream_to_lang(nav, lg);
        if (lang != 0xFFFF)
        {
            tmp[0] = lang >> 8;
            tmp[1] = lang & 0xFF;
            tmp[2] = 0;
        }
        format = dvdnav_audio_stream_format(nav, lg);
        if (format == 0xFFFF || format > 6)
            format = 1; //unknown
        id = i + base[format];
        AVDebug("audio stream: %d format: %s (%s) language: %s aid: %d.\n", i,
            dvd_audio_stream_types[format], dvd_audio_stream_channels[channels], tmp, id);
        if (lang != 0xFFFF && lang && tmp[0])
            AVDebug("ID_AID_%d_LANG=%s\n", id, tmp);
    }

    for (i = 0; i < 32; i++)
    {
        char tmp[] = "unknown";
        lg = dvdnav_get_spu_logical_stream(nav, i);
        if (lg == 0xff) continue;
        lang = dvdnav_spu_stream_to_lang(nav, i);
        if (lang != 0xFFFF)
        {
            tmp[0] = lang >> 8;
            tmp[1] = lang & 0xFF;
            tmp[2] = 0;
        }
        AVDebug("subtitle ( sid ): %d language: %s\n", lg, tmp);
        if (lang != 0xFFFF && lang && tmp[0])
            AVDebug("ID_SID_%d_LANG=%s\n", lg, tmp);
    }
}

static void dvdnav_get_highlight(dvdnav_priv_t *priv, int display_mode) {
    pci_t *pnavpci = NULL;
    dvdnav_highlight_event_t *hlev = &(priv->hlev);
    int btnum;

    if (!priv || !priv->dvdnav)
        return;

    pnavpci = dvdnav_get_current_nav_pci(priv->dvdnav);
    if (!pnavpci)
        return;

    dvdnav_get_current_highlight(priv->dvdnav, (int32_t*)&(hlev->buttonN));
    hlev->display = display_mode; /* show */

    if (hlev->buttonN > 0 && pnavpci->hli.hl_gi.btn_ns > 0 && hlev->display) {
        for (btnum = 0; btnum < pnavpci->hli.hl_gi.btn_ns; btnum++) {
            btni_t *btni = &(pnavpci->hli.btnit[btnum]);

            if (hlev->buttonN == btnum + 1) {
                hlev->sx = FFMIN(btni->x_start, btni->x_end);
                hlev->ex = FFMAX(btni->x_start, btni->x_end);
                hlev->sy = FFMIN(btni->y_start, btni->y_end);
                hlev->ey = FFMAX(btni->y_start, btni->y_end);

                hlev->palette = (btni->btn_coln == 0) ? 0 :
                    pnavpci->hli.btn_colit.btn_coli[btni->btn_coln - 1][0];
                break;
            }
        }
    }
    else { /* hide button or no button */
        hlev->sx = hlev->ex = 0;
        hlev->sy = hlev->ey = 0;
        hlev->palette = hlev->buttonN = 0;
    }
}

class DVDIOPrivate : public MediaIOPrivate
{
public:
    DVDIOPrivate() : MediaIOPrivate(),
        //dvd_stream(NULL),
        priv(NULL),
        track(0),
        dvd_angle(0),
        dvd_last_chapter(1),
        current_title(0),
        current_chapter(0),
        current_chapter_index(0),
        started(false),
        current_start_time(0),
        current_time(0),
        current_duration(0),
        custom_duration(0),
        current_cell(0),
        file_sys(UDF),
        skip_nav_pack(false),
        stopped(true)
    {

    }
    ~DVDIOPrivate()
    {
        close();
    }

    int open(const std::string &file);
    void readInfo();
    int read(unsigned char *buf, int len);
    bool seek(int64_t newpos);
    void close();
    void update_title_len();
    void stream_dvdnav_get_time();
    int64_t update_current_time();

public:
    int fd;   // file descriptor, see man open(2)
    int type; // see STREAMTYPE_*
    int flags;
    int sector_size; // sector size (seek will be aligned on this size if non 0)
    int read_chunk; // maximum amount of data to read at once to limit latency (0 for default)
    unsigned int buf_pos, buf_len;
    int64_t pos, start_pos, end_pos;
    int eof;
    int mode; //STREAM_READ or STREAM_WRITE
    string url;  // strdup() of filename/url
    dvdnav_priv_t *priv;

    /**/
    int track;

public:
    //stream *dvd_stream;
    int current_title;
    int current_chapter;
    int current_chapter_index;
    std::list<int> parts;
    int dvd_angle;
    int dvd_last_chapter;
    bool started;
    int64_t current_start_time;
    int64_t current_time;
    int64_t current_duration;
    int64_t custom_duration;
    int current_cell;
    file_system file_sys;
    int error_count;
    int clock_flag;

    bool skip_nav_pack;
    bool stopped;

private:

};

int DVDIOPrivate::open(const std::string &file)
{
    dvdnav_status_t status = DVDNAV_STATUS_ERR;
    const char *titleName;
    int titles = 0;
    const char* filesystem[] = { "auto", "udf", "iso9660" };

    clock_flag = 2;

    if (!(priv = (dvdnav_priv_t*)calloc(1, sizeof(dvdnav_priv_t))))
        return STREAM_UNSUPPORTED;

    if (dvdnav_open(&(priv->dvdnav), file.c_str()) != DVDNAV_STATUS_OK || !priv->dvdnav) {
        delete priv;
        priv = NULL;
        return STREAM_UNSUPPORTED;
    }
    if (1) {	//from vlc: if not used dvdnav from cvs will fail
        int len, event;
        uint8_t buf[2048];
        dvdnav_get_next_block(priv->dvdnav, buf, &event, &len);
        dvdnav_sector_search(priv->dvdnav, 0, SEEK_SET);
    }
    /* turn off dvdnav caching */
    dvdnav_set_readahead_flag(priv->dvdnav, 0);
    if (dvdnav_set_PGC_positioning_flag(priv->dvdnav, 1) != DVDNAV_STATUS_OK)
        AVDebug("stream_dvdnav, failed to set PGC positioning\n");

    /* report the title?! */
    if (dvdnav_get_title_string(priv->dvdnav, &titleName) == DVDNAV_STATUS_OK) {
        AVDebug("ID_DVD_VOLUME_ID=%s\n", titleName);
    }

    if (track > 0) {
        priv->title = track;
        if (dvdnav_title_play(priv->dvdnav, track) != DVDNAV_STATUS_OK) {
            AVDebug("dvdnav_stream, couldn't select title %d, error '%s'\n", track, dvdnav_err_to_string(priv->dvdnav));
            close();
            return STREAM_UNSUPPORTED;
        }
        AVDebug("ID_DVD_CURRENT_TITLE=%d\n", track);
    }
    else if (track == 0) {
        if (dvdnav_menu_call(priv->dvdnav, DVD_MENU_Root) != DVDNAV_STATUS_OK)
            dvdnav_menu_call(priv->dvdnav, DVD_MENU_Title);
    }
    //readInfo();
    return STREAM_OK;
}

void DVDIOPrivate::readInfo()
{
    uint32_t titles = 0, i;
    if (track <= 0) {
        dvdnav_get_number_of_titles(priv->dvdnav, (int32_t*)&titles);
        for (i = 1; i <= titles; i++) {
            identify_chapters(priv->dvdnav, i);
        }
    }
    else
        identify_chapters(priv->dvdnav, track);
    if (current_title > 0)
        show_audio_subs_languages(priv->dvdnav);
    if (dvd_angle > 1)
        dvdnav_angle_change(priv->dvdnav, dvd_angle);

    sector_size = 2048;
    type = STREAMTYPE_DVDNAV;

    update_title_len();
    if (!pos && current_title > 0)
        AVDebug("INIT ERROR: couldn't get init pos %s\r\n", dvdnav_err_to_string(priv->dvdnav));

    AVDebug("Remember to disable MPlayer's cache when playing dvdnav:// streams (adding -nocache to your command line)\r\n");

}

int DVDIOPrivate::read(unsigned char *buffer, int len)
{
    int event;

    if (priv->state & NAV_FLAG_WAIT_READ) /* read is suspended */
        return -1;
    len = 0;
    if (!end_pos)
        update_title_len();
    while (!len) /* grab all event until DVDNAV_BLOCK_OK (len=2048), DVDNAV_STOP or DVDNAV_STILL_FRAME */
    {
        event = dvdnav_stream_read(priv, buffer, &len);
        if (event == -1 || len == -1)
        {
            AVDebug("DVDNAV stream read error!\n");
            return 0;
        }
        if (event != DVDNAV_BLOCK_OK)
            dvdnav_get_highlight(priv, 1);
        switch (event) {
        case DVDNAV_STILL_FRAME: {
            dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)buffer;
            priv->still_length = still_event->length;
            /* set still frame duration */
            priv->duration = dvdnav_get_duration(priv->still_length);
            if (priv->still_length <= 1) {
                pci_t *pnavpci = dvdnav_get_current_nav_pci(priv->dvdnav);
                priv->duration = dvdtimetomsec(&pnavpci->pci_gi.e_eltm);
            }
            if (still_event->length < 0xff)
                printf("Skipping %d seconds of still frame\n", still_event->length);
            else {
                printf("Skipping indefinite length still frame\n");
                dvdnav_still_skip(priv->dvdnav);
            }
            //break;
            return 0;
        }
        case DVDNAV_HIGHLIGHT: {
            dvdnav_get_highlight(priv, 1);
            break;
        }
        case DVDNAV_SPU_CLUT_CHANGE: {
            memcpy(priv->spu_clut, buffer, 16 * sizeof(unsigned int));
            priv->state |= NAV_FLAG_SPU_SET;
            break;
        }
        case DVDNAV_STOP: {
            priv->state |= NAV_FLAG_EOF;
            return len;
        }
        case DVDNAV_BLOCK_OK:
        case DVDNAV_NAV_PACKET:
            return len;
        case DVDNAV_WAIT: {
            //if ((priv->state & NAV_FLAG_WAIT_SKIP) &&
            //    !(priv->state & NAV_FLAG_WAIT))
                dvdnav_wait_skip(priv->dvdnav);
            //else
            //    priv->state |= NAV_FLAG_WAIT;
            //if (priv->state & NAV_FLAG_WAIT)
            //    return len;
                printf("------------------dvdnav_wait_skip!!!------------\n");
            break;
        }
        case DVDNAV_VTS_CHANGE: {
            int tit = 0, part = 0;
            dvdnav_vts_change_event_t *vts_event = (dvdnav_vts_change_event_t *)buffer;
            AVDebug("DVDNAV, switched to title: %d\r\n", vts_event->new_vtsN);
            priv->state |= NAV_FLAG_CELL_CHANGE;
            priv->state |= NAV_FLAG_AUDIO_CHANGE;
            priv->state |= NAV_FLAG_SPU_CHANGE;
            priv->state |= NAV_FLAG_STREAM_CHANGE;
            priv->state &= ~NAV_FLAG_WAIT_SKIP;
            priv->state &= ~NAV_FLAG_WAIT;
            end_pos = 0;
            update_title_len();
            show_audio_subs_languages(priv->dvdnav);
            if (priv->state & NAV_FLAG_WAIT_READ_AUTO)
                priv->state |= NAV_FLAG_WAIT_READ;
            if (dvdnav_current_title_info(priv->dvdnav, &tit, &part) == DVDNAV_STATUS_OK) {
                AVDebug("\r\nDVDNAV, NEW TITLE %d\r\n", tit);
                dvdnav_get_highlight(priv, 0);
                if (priv->title > 0 && tit != priv->title) {
                    priv->state |= NAV_FLAG_EOF;
                    return 0;
                }
            }
            break;
        }
        case DVDNAV_CELL_CHANGE: {
            dvdnav_cell_change_event_t *ev = (dvdnav_cell_change_event_t*)buffer;
            uint32_t nextstill;

            priv->state &= ~NAV_FLAG_WAIT_SKIP;
            priv->state |= NAV_FLAG_STREAM_CHANGE;
            if (ev->pgc_length)
                priv->duration = ev->pgc_length / 90;

            if (dvdnav_is_domain_vts(priv->dvdnav)) {
                AVDebug("DVDNAV_TITLE_IS_MOVIE\n");
                priv->state &= ~NAV_FLAG_VTS_DOMAIN;
            }
            else {
                AVDebug("DVDNAV_TITLE_IS_MENU\n");
                priv->state |= NAV_FLAG_VTS_DOMAIN;
            }

            nextstill = dvdnav_get_next_still_flag(priv->dvdnav);
            if (nextstill) {
                priv->duration = dvdnav_get_duration(nextstill);
                priv->still_length = nextstill;
                if (priv->still_length <= 1) {
                    pci_t *pnavpci = dvdnav_get_current_nav_pci(priv->dvdnav);
                    priv->duration = dvdtimetomsec(&pnavpci->pci_gi.e_eltm);
                }
            }

            priv->state |= NAV_FLAG_CELL_CHANGE;
            priv->state |= NAV_FLAG_AUDIO_CHANGE;
            priv->state |= NAV_FLAG_SPU_CHANGE;
            priv->state &= ~NAV_FLAG_WAIT_SKIP;
            priv->state &= ~NAV_FLAG_WAIT;
            if (priv->state & NAV_FLAG_WAIT_READ_AUTO)
                priv->state |= NAV_FLAG_WAIT_READ;
            if (priv->title > 0 && dvd_last_chapter > 0) {
                int tit = 0, part = 0;
                if (dvdnav_current_title_info(priv->dvdnav, &tit, &part) == DVDNAV_STATUS_OK && part > dvd_last_chapter) {
                    priv->state |= NAV_FLAG_EOF;
                    return 0;
                }
            }
            dvdnav_get_highlight(priv, 1);
        }
                                 break;
        case DVDNAV_AUDIO_STREAM_CHANGE:
            priv->state |= NAV_FLAG_AUDIO_CHANGE;
            break;
        case DVDNAV_SPU_STREAM_CHANGE:
            priv->state |= NAV_FLAG_SPU_CHANGE;
            priv->state |= NAV_FLAG_STREAM_CHANGE;
            break;
        }
    }
    AVDebug("DVDNAV fill_buffer len: %d\n", len);
    return len;
}

bool DVDIOPrivate::seek(int64_t newpos)
{
    uint64_t sector = 0;

    if (end_pos && newpos > end_pos)
        newpos = end_pos;
    sector = newpos / 2048ULL;
    if (dvdnav_sector_search(priv->dvdnav, (uint64_t)sector, SEEK_SET) != DVDNAV_STATUS_OK)
        return false;
    pos = newpos;
    return true;
}

void DVDIOPrivate::update_title_len()
{
    dvdnav_status_t status;
    uint32_t pos = 0, len = 0;

    status = dvdnav_get_position(priv->dvdnav, &pos, &len);
    if (status == DVDNAV_STATUS_OK && len) {
        end_pos = len * 2048ull;
    }
    else {
        end_pos = 0;
    }
}

int64_t DVDIOPrivate::update_current_time()
{
    int64_t time = FORCE_INT64(dvdnav_get_current_time(priv->dvdnav) / 90.0f);
    if (time < 0)
        return current_time;
    time -= current_start_time;
    if (time != current_time) {
        current_time = time;
    }
    if (!started) {
        current_start_time = time;
        started = true;
    }
    return current_time;
}

#ifdef _WIN32
static int utf8_to_mb(char* str_utf8, char* local_buf, size_t len) {
    int widelen;
    wchar_t *widestr;
    int mblen;

    widelen = MultiByteToWideChar(CP_UTF8, 0, str_utf8, strlen(str_utf8), NULL, 0);
    widestr = (wchar_t *)malloc(sizeof(wchar_t)*(widelen + 1));
    if (widestr == NULL) {
        return 0;
    }
    MultiByteToWideChar(CP_UTF8, 0, str_utf8, strlen(str_utf8), widestr, widelen);

    mblen = WideCharToMultiByte(CP_ACP, 0, widestr, widelen, NULL, 0, NULL, NULL);
    if (mblen + 1 > len) {
        free(widestr);
        return 0;
    }
    WideCharToMultiByte(CP_ACP, 0, widestr, widelen, local_buf, mblen, NULL, NULL);
    local_buf[mblen] = '\0';
    free(widestr);
    return 1;
}
#endif

void DVDIOPrivate::close()
{
    if (priv) {
        priv->duration = 0;
        stopped = true;
        if (priv->dvdnav) dvdnav_close(priv->dvdnav);
        delete priv;
        priv = NULL;
    }
    current_title = 0;
    started = false;
    current_start_time = 0;
    current_time = 0;
    current_duration = 0;
    custom_duration = 0;
    current_cell = 0;
}

void DVDIOPrivate::stream_dvdnav_get_time()
{
    uint64_t *parts = NULL, duration = 0;
    uint32_t n;
    int64_t title_duration = 0;

    n = dvdnav_describe_title_chapters(priv->dvdnav, current_title, &parts, &duration);
    if (parts) {
        title_duration = duration / 90;
        title_duration *= 1000;
    }
    if (priv->duration > 0) {
        current_duration = (int64_t)priv->duration * 1000;
    }
    else {
        current_duration = title_duration;
    }
}

NAMESPACE_END

#endif
