#include "mediaio.h"
#include "private/mediaio_p.h"
#include "AVLog.h"
#include "mkid.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "dvdread/dvd_reader.h"
#include "dvdread/ifo_types.h"
#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"
#include "dvdnav/dvd_types.h"
#include "dvdnav/dvdnav.h"
#ifdef __cplusplus
}
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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

NAMESPACE_BEGIN

class DVDIOPrivate;
typedef struct stream {
    int fd;   // file descriptor, see man open(2)
    int type; // see STREAMTYPE_*
    int flags;
    int sector_size; // sector size (seek will be aligned on this size if non 0)
    int read_chunk; // maximum amount of data to read at once to limit latency (0 for default)
    unsigned int buf_pos, buf_len;
    int64_t pos, start_pos, end_pos;
    int eof;
    int mode; //STREAM_READ or STREAM_WRITE
    char* url;  // strdup() of filename/url
} stream_t;

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

enum file_system {
    AUTO = 0,
    UDF,
    ISO9660
};

class DVDIO : public MediaIO
{
    DPTR_DECLARE_PRIVATE(DVDIO)
public:
    DVDIO();
    virtual std::string name() const;

    const std::list<std::string>& protocols() const
    {
        static std::list<std::string> p;
        p.push_back("dvd");
        return p;
    }

    virtual bool isSeekable() const override;
    virtual bool isWritable() const override;
    virtual int64_t read(char *data, int64_t maxSize) override;
    virtual int64_t write(const char *data, int64_t maxSize) override;
    virtual bool seek(int64_t offset, int from) override;
    virtual int64_t position() const override;
    /*!
    * \brief size
    * \return <=0 if not support
    */
    virtual int64_t size() const override;
    virtual int64_t clock() override;
    virtual int64_t startTimeUs() override;
    virtual int64_t duration() override;
    virtual void setDuration(int64_t duration) override;
    virtual void setOptionsForIOCodec(const StringMap & dict) override;

protected:
    DVDIO(DVDIOPrivate* d);

    void onUrlChanged() override;

};
typedef DVDIO MediaIODVD;
static const MediaIOId MediaIOId_DVD = mkid::id32base36_3<'d', 'v', 'd'>::value;
static const char kDVDDevName[] = "dvd";
FACTORY_REGISTER(MediaIO, DVD, kDVDDevName)

class DVDIOPrivate : public MediaIOPrivate
{
public:
    DVDIOPrivate() : MediaIOPrivate(),
        dvd_stream(NULL),
        priv(NULL),
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
        stream_dvdnav_close();
    }
    int  stream_dvdnav_open(const char *filename);
    int  stream_dvdnav_read(stream_t *s, char *but, int len);
    bool stream_dvdnav_seek(stream_t *s, int64_t newpos);
    void stream_dvdnav_close();
    void stream_dvdnav_get_time();
    int64_t update_current_time();

public:
    stream *dvd_stream;
    dvdnav_priv_t *priv;
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
    stream_t* new_stream(int fd, int type);
    int  dvdnav_first_play();
    int  dvdnav_get_duration(int length);
    int  dvdnav_stream_read(dvdnav_priv_t * priv, unsigned char *buf, int *len);
    void update_title_len(stream_t *stream);
    int  dvdtimetomsec(dvd_time_t *dt);
};

DVDIO::DVDIO()
    :MediaIO(new DVDIOPrivate())
{
}
DVDIO::DVDIO(DVDIOPrivate* d):
    MediaIO(d)
{
}
std::string DVDIO::name() const { return kDVDDevName; }

bool DVDIO::isSeekable() const
{
    return true;
}

bool DVDIO::isWritable() const
{
    return false;
}

int64_t DVDIO::read(char *data, int64_t maxSize)
{
    DPTR_D(DVDIO);

    PU_UNUSED(maxSize)
        if (!d->dvd_stream)
            return AVERROR_EOF;
    int len = 0;

    len = d->stream_dvdnav_read(d->dvd_stream, data, STREAM_BUFFER_SIZE);
    if (len <= 0) {
        return AVERROR_EOF;
    }
    return static_cast<int64_t>(len);
}

int64_t DVDIO::write(const char *data, int64_t maxSize)
{
    PU_UNUSED(data);
    PU_UNUSED(maxSize);
    return 0;
}

bool DVDIO::seek(int64_t offset, int from)
{
    DPTR_D(DVDIO);

    PU_UNUSED(from);
    if (!d->dvd_stream)
        return false;

    if (from == -200) {
        uint32_t newpos, len;
        if (dvdnav_time_search(d->priv->dvdnav, offset * 90) != DVDNAV_STATUS_OK)
            return false;
        char buf[STREAM_BUFFER_SIZE];
        dvdnav_status_t status = DVDNAV_STATUS_ERR;
        while (status != DVDNAV_STATUS_OK) {
            d->stream_dvdnav_read(d->dvd_stream, buf, STREAM_BUFFER_SIZE);
            status = dvdnav_get_position(d->priv->dvdnav, &newpos, &len);
        }
        d->dvd_stream->pos = newpos;
        return true;
    }

    bool flag = d->stream_dvdnav_seek(d->dvd_stream, offset);
    return flag;
}

int64_t DVDIO::position() const
{
    DPTR_D(const DVDIO);
    if (!d->dvd_stream)
        return 0;
    return d->dvd_stream->pos;
}

int64_t DVDIO::size() const
{
    DPTR_D(const DVDIO);
    if (!d->dvd_stream)
        return 0;
    return d->dvd_stream->end_pos - d->dvd_stream->start_pos; // sequential device returns bytesAvailable()
}

int64_t DVDIO::clock()
{
    DPTR_D(DVDIO);

    int64_t time = 0;

    if (!d->dvd_stream || !d->priv)
        return 0;
    if (d->priv && d->stopped) {
        return d->current_duration / 1000;
    }
    time = d->update_current_time();
    if (d->clock_flag) {
        --d->clock_flag;
        return 0;
    }
    return time;/*ms*/
}

int64_t DVDIO::startTimeUs()
{
    DPTR_D(DVDIO);

    return d->current_start_time;
}

int64_t DVDIO::duration()
{
    DPTR_D(DVDIO);

    return d->current_duration;
}

void DVDIO::setDuration(int64_t duration)
{
    DPTR_D(DVDIO);

    d->custom_duration = duration;
}

void DVDIO::setOptionsForIOCodec(const StringMap &dict)
{
    DPTR_D(DVDIO);

    //QVariant opt(dict);
    //if (dict.contains("filesys")) {
    //    int sys = dict.value("filesys").toInt();
    //    if (sys >= AUTO && sys <= ISO9660) {
    //        d->file_sys = (file_system)sys;
    //    }
    //}
    //if (dict.find("title") != dict.end()) {
    //    d->current_title = dict.("title").toInt();
    //}
    //if (dict.contains("chapter")) {
    //    QList<QVariant> lstParts = dict.value("chapter").toList();
    //    d->parts.clear();
    //    for (int i = 0; i < lstParts.count(); ++i) {
    //        d->parts.push_back(lstParts.at(i).toInt());
    //    }
    //    std::sort(d->parts.begin(), d->parts.end(), [&](int part1, int part2)->bool {
    //        return part1 < part2; });
    //}
}

void DVDIO::onUrlChanged()
{
    DPTR_D(DVDIO);
    std::string path(url());
    std::string filename, titleInfo;

    if (path.substr(0, 6) == "dvd://") {
        path = path.substr(7, path.size()); //remove "dvd://"
        d->stream_dvdnav_open(path.data());
    }
}

stream_t* DVDIOPrivate::new_stream(int fd, int type)
{
    stream_t *s = (stream_t *)calloc(1, sizeof(stream_t));
    if (s == NULL) return NULL;

    s->fd = fd;
    s->type = type;
    s->buf_pos = s->buf_len = 0;
    s->start_pos = s->end_pos = 0;
    if (s->eof) {
        s->buf_pos = s->buf_len = 0;
        s->eof = 0;
    }
    return s;
}

int DVDIOPrivate::dvdnav_get_duration(int length)
{
    return (length == 255) ? 0 : length * 1000;
}

int DVDIOPrivate::dvdtimetomsec(dvd_time_t *dt)
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

int DVDIOPrivate::dvdnav_stream_read(dvdnav_priv_t * priv, unsigned char *buf, int *len)
{
    int event = DVDNAV_NOP;

    if (dvdnav_get_next_block(priv->dvdnav, buf, &event, len) != DVDNAV_STATUS_OK) {
        ++error_count;
        if (dvdnav_sector_search(priv->dvdnav, 1, SEEK_CUR) == DVDNAV_STATUS_ERR) {
            AVDebug("sector search error : %s\n", dvdnav_err_to_string(priv->dvdnav));
            *len = 0;
        }
    }
    if (event != DVDNAV_BLOCK_OK && event != DVDNAV_NAV_PACKET) {
        *len = 0;
    }
    return event;
}

int DVDIOPrivate::stream_dvdnav_read(stream_t *s, char *buf, int len)
{
    int event;

    if (current_title > 0 && stopped)
        return 0;
    len = 0;
    if (!s->end_pos)
        update_title_len(s);
    error_count = 0;
    while (!len) /* grab all event until DVDNAV_BLOCK_OK (len=2048), DVDNAV_STOP or DVDNAV_STILL_FRAME */
    {
        if (error_count >= 500) {
            AVDebug() << "Too many consecutive errors read!\n";
            return 0;
        }
        event = dvdnav_stream_read(priv, (unsigned char*)buf, &len);
        if (event == -1 || len == -1) {
            AVDebug("DVDNAV stream read error!\n");
            return 0;
        }
        switch (event) {
        case DVDNAV_STILL_FRAME: {
            pci_t *pci;
            dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)buf;
            if (still_event->length < 0xff)
                AVDebug("Skipping %d seconds of still frame\n", still_event->length);
            else
                AVDebug("Skipping indefinite length still frame\n");
            pci = dvdnav_get_current_nav_pci(priv->dvdnav);
            dvdnav_button_activate(priv->dvdnav, pci);
            dvdnav_still_skip(priv->dvdnav);
            break;
        }
        case DVDNAV_HIGHLIGHT: {
            AVDebug("DVDNAV_HIGHLIGHT");
            break;
        }
        case DVDNAV_SPU_CLUT_CHANGE: {
            memcpy(priv->spu_clut, buf, 16 * sizeof(unsigned int));
            break;
        }
        case DVDNAV_STOP: {
            stopped = true;
            return len;
        }
        case DVDNAV_NAV_PACKET:
            pci_t *pci;
            pci = dvdnav_get_current_nav_pci(priv->dvdnav);
            dvdnav_get_current_nav_dsi(priv->dvdnav);

            if (pci->hli.hl_gi.btn_ns > 0) {
                int button;
                AVDebug("Found %i DVD menu buttons...\n", pci->hli.hl_gi.btn_ns);

                for (button = 0; button < pci->hli.hl_gi.btn_ns; button++) {
                    btni_t *btni = &(pci->hli.btnit[button]);
                    AVDebug("Button %i top-left @ (%i,%i), bottom-right @ (%i,%i)\n",
                        button + 1, btni->x_start, btni->y_start,
                        btni->x_end, btni->y_end);
                }

                button = 0;
                while ((button <= 0) || (button > pci->hli.hl_gi.btn_ns)) {
                    AVDebug("Which button (1 to %i): ", pci->hli.hl_gi.btn_ns);
                    scanf("%i", &button);
                    if (button == 100) {
                        stopped = true;
                        button = 0;
                        break;
                    }
                    else if (button == 200) {
                        skip_nav_pack = true;
                        button = 0;
                        break;
                    }
                }

                AVDebug("Selecting button %i...\n", button);
                if (button) {
                    dvdnav_button_select_and_activate(priv->dvdnav, pci, button);
                }
            }
            break;
        case DVDNAV_BLOCK_OK:
            return len;
        case DVDNAV_WAIT: {
            AVDebug("DVDNAV_WAIT");
            dvdnav_wait_skip(priv->dvdnav);
            break;
        }
        case DVDNAV_VTS_CHANGE: {
            int tit = 0, part = 0;
            current_cell = 0;
            dvdnav_vts_change_event_t *vts_event = (dvdnav_vts_change_event_t *)buf;
            AVDebug("DVDNAV, switched to title: %d\r\n", vts_event->new_vtsN);
            s->end_pos = 0;
            update_title_len(s);
            if (dvdnav_current_title_info(priv->dvdnav, &tit, &part) == DVDNAV_STATUS_OK) {
                AVDebug("DVDNAV, NEW TITLE %d->", tit);
                if (started && current_title > 0 && tit != current_title) {
                    stopped = true;
                    return 0;
                }
                /*Fix bug, some iso's title switch to 0 2018-05-31*/
                if (tit == 0) {
                    dvdnav_title_play(priv->dvdnav, current_title);
                }
            }
            break;
        }
        case DVDNAV_CELL_CHANGE: {
            dvdnav_cell_change_event_t *ev = (dvdnav_cell_change_event_t*)buf;
            uint32_t nextstill;
            if (ev->pgc_length)
                priv->duration = FORCE_UINT(ev->pgc_length / 90);

            if (dvdnav_is_domain_vts(priv->dvdnav)) {
                AVDebug("DVDNAV_TITLE_IS_MOVIE\n");
            }
            else {
                AVDebug("DVDNAV_TITLE_IS_MENU\n");
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
            return len;
            break;
        }
        case DVDNAV_AUDIO_STREAM_CHANGE:
            AVDebug("DVDNAV_AUDIO_STREAM_CHANGE\n");
            break;
        case DVDNAV_SPU_STREAM_CHANGE:
            AVDebug("DVDNAV_SPU_STREAM_CHANGE\n");
            break;
        }
    }
    return len;
}

bool DVDIOPrivate::stream_dvdnav_seek(stream_t *s, int64_t newpos)
{
    uint64_t sector = 0;

    if (s->end_pos && newpos > s->end_pos)
        newpos = s->end_pos;
    sector = newpos / 2048ULL;
    if (dvdnav_sector_search(priv->dvdnav, (uint64_t)sector, SEEK_SET) != DVDNAV_STATUS_OK)
        return false;
    s->pos = newpos;
    return true;
}

int DVDIOPrivate::dvdnav_first_play()
{
    current_chapter_index = 0;
    if (current_title > 0) {
        if (dvdnav_title_play(priv->dvdnav, current_title) != DVDNAV_STATUS_OK) {
            AVDebug("dvdnav_stream, couldn't select title %d, error '%s'\n", current_title, dvdnav_err_to_string(priv->dvdnav));
            stream_dvdnav_close();
            dvd_stream = NULL;
            return STREAM_UNSUPPORTED;
        }
        AVDebug("ID_DVD_CURRENT_TITLE=%d\n", current_title);
    }
    else if (current_title == 0) {
        if (dvdnav_menu_call(priv->dvdnav, DVD_MENU_Root) != DVDNAV_STATUS_OK)
            dvdnav_menu_call(priv->dvdnav, DVD_MENU_Title);
    }
    return STREAM_OK;
}

void DVDIOPrivate::update_title_len(stream_t *s)
{
    dvdnav_status_t status;
    uint32_t pos = 0, len = 0;

    status = dvdnav_get_position(priv->dvdnav, &pos, &len);
    if (status == DVDNAV_STATUS_OK && len) {
        s->end_pos = len * 2048ull;
    }
    else {
        s->end_pos = 0;
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

int DVDIOPrivate::stream_dvdnav_open(const char *file)
{
    dvdnav_status_t status = DVDNAV_STATUS_ERR;
    const char *titleName;
    int titles = 0;
    char filename[2048];
    char *temp = strdup(file);
    const char* filesystem[] = { "auto", "udf", "iso9660" };

    clock_flag = 2;
    memset(filename, 0, 2048);
#ifdef _WIN32
    if (!utf8_to_mb(temp, filename, 2048)) {
        free(temp);
        return STREAM_UNSUPPORTED;
    }
#endif
    free(temp);
    if (dvd_stream == NULL) {
        dvd_stream = new_stream(-2, -2);
        dvd_stream->flags |= STREAM_READ;
    }

    if (!(priv = (dvdnav_priv_t*)calloc(1, sizeof(dvdnav_priv_t))))
        return STREAM_UNSUPPORTED;

    //DVDReadMode(file_sys);
    if (dvdnav_open(&(priv->dvdnav), filename) != DVDNAV_STATUS_OK || !priv->dvdnav) {
        delete dvd_stream;
        dvd_stream = NULL;
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

    status = dvdnav_get_number_of_titles(priv->dvdnav, &titles);
    if (status != DVDNAV_STATUS_OK || current_title > titles) {
        current_title = 0;
    }

    if (dvdnav_first_play() != STREAM_OK) {
        return STREAM_UNSUPPORTED;
    }
    stopped = false;

    if (dvd_angle > 1)
        dvdnav_angle_change(priv->dvdnav, dvd_angle);

    dvd_stream->sector_size = 2048;
    dvd_stream->flags = STREAM_READ;
    dvd_stream->type = STREAMTYPE_DVDNAV;

    char buf[STREAM_BUFFER_SIZE];
    int ret = 0;
    while (dvd_stream->end_pos == 0) {
        ret = stream_dvdnav_read(dvd_stream, buf, STREAM_BUFFER_SIZE);
        if (ret <= 0) {
            /*Prevent forever loops if the DVD file is invalid->*/
            break;
        }
    }
    if (!dvdnav_first_play()) {
        return STREAM_UNSUPPORTED;
    }

    if (!dvd_stream->pos && current_title > 0)
        AVDebug() << "INIT ERROR: couldn't get init pos %s\r\n" << dvdnav_err_to_string(priv->dvdnav);
    stream_dvdnav_get_time();

    return STREAM_OK;
}

void DVDIOPrivate::stream_dvdnav_close()
{
    if (dvd_stream) {
        delete dvd_stream;
        dvd_stream = NULL;
    }
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
    int64_t title_duration;

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