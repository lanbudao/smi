#include "mediaio.h"
#include "private/mediaio_p.h"
#include "utils/AVLog.h"
#include "mkid.h"
#include "dvdio.h"
#include "dvdio_p.h"
#include "sdk/io/dvdagency.h"

NAMESPACE_BEGIN

typedef DVDIO MediaIODVD;
static const MediaIOId MediaIOId_DVD = mkid::id32base36_3<'d', 'v', 'd'>::value;
static const char kDVDDevName[] = "dvd";
FACTORY_REGISTER(MediaIO, DVD, kDVDDevName)

DVDIO::DVDIO():
    MediaIO(new DVDIOPrivate)
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

int DVDIO::read(unsigned char *data, int64_t maxSize)
{
    DPTR_D(DVDIO);
    PU_UNUSED(maxSize)

    int len = 0;
    len = d->read(data, STREAM_BUFFER_SIZE);
    if (len == 0) {
        return AVERROR_UNKNOWN;
    }
    else if (len < 0) {
        return AVERROR_EOF;
    }
    return len;
}

int DVDIO::write(const unsigned char *data, int64_t maxSize)
{
    PU_UNUSED(data);
    PU_UNUSED(maxSize);
    return 0;
}

bool DVDIO::seek(int64_t offset, int from)
{
    DPTR_D(DVDIO);

    PU_UNUSED(from);

    if (from == -200) {
        uint32_t newpos, len;
        if (dvdnav_time_search(d->priv->dvdnav, offset * 90) != DVDNAV_STATUS_OK)
            return false;
        unsigned char buf[STREAM_BUFFER_SIZE];
        dvdnav_status_t status = DVDNAV_STATUS_ERR;
        while (status != DVDNAV_STATUS_OK) {
            d->read(buf, STREAM_BUFFER_SIZE);
            status = dvdnav_get_position(d->priv->dvdnav, &newpos, &len);
        }
        d->pos = newpos;
        return true;
    }

    bool flag = d->seek(offset);
    return flag;
}

int64_t DVDIO::position() const
{
    DPTR_D(const DVDIO);
    return d->pos;
}

int64_t DVDIO::size() const
{
    DPTR_D(const DVDIO);
    return d->end_pos - d->start_pos; // sequential device returns bytesAvailable()
}

int64_t DVDIO::clock()
{
    DPTR_D(DVDIO);

    int64_t time = 0;

    if (!d->priv)
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
        path = path.substr(6, path.size()); //remove "dvd://"
        d->open(path);
    }
}

int DVDIO::setMouseActive(int x, int y)
{
    return 0;
}
int DVDIO::setMouseSelect(int x, int y)
{

    return 0;
}

int DVDIO::setUpperButtonSelect()
{

    return 0;
}

int DVDIO::setLowerButtonSelect()
{

    return 0;
}

int DVDIO::setLeftButtonSelect()
{

    return 0;
}

int DVDIO::setRightButtonSelect()
{

    return 0;
}

class DVDAgencyPrivate
{
public:
    DVDIO* dvd_io;
};

DVDAgency::DVDAgency(MediaIO* io):
    d_ptr(new DVDAgencyPrivate)
{
    DPTR_D(DVDAgency);
    d->dvd_io = dynamic_cast<DVDIO*>(io);
}

DVDAgency::~DVDAgency()
{

}

int DVDAgency::setMouseActive(int x, int y)
{
    DPTR_D(DVDAgency);
    return d->dvd_io->setMouseActive(x, y);
}

int DVDAgency::setMouseSelect(int x, int y)
{
    DPTR_D(DVDAgency);
    return d->dvd_io->setMouseSelect(x, y);
}

int DVDAgency::setUpperButtonSelect()
{
    DPTR_D(DVDAgency);
    return d->dvd_io->setUpperButtonSelect();
}

int DVDAgency::setLowerButtonSelect()
{
    DPTR_D(DVDAgency);
    return d->dvd_io->setLowerButtonSelect();
}

int DVDAgency::setLeftButtonSelect()
{
    DPTR_D(DVDAgency);
    return d->dvd_io->setLeftButtonSelect();
}

int DVDAgency::setRightButtonSelect()
{
    DPTR_D(DVDAgency);
    return d->dvd_io->setRightButtonSelect();
}

NAMESPACE_END