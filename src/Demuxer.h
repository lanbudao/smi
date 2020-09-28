#ifndef SMP_DEMUXER_H
#define SMP_DEMUXER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "sdk/mediainfo.h"
#include "Packet.h"

#include <string>

struct AVFormatContext;
typedef struct AVStream AVStream;

NAMESPACE_BEGIN

struct AVCodecContext;
class DemuxerPrivate;
class FFPRO_EXPORT Demuxer
{
    DPTR_DECLARE_PRIVATE(Demuxer)
public:
    Demuxer();
    ~Demuxer();

    enum ErrorCode {
        NoError,
        OpenError,
        FindStreamError,
    };

    void setMedia(const char* url);
    void setWantedStreamSpec(MediaType type, const char* spec);
    void setMediaStreamDisable(MediaType type);
    int  load();
    void unload();
    bool atEnd();
    void setEOF(bool eof);
    bool hasAttachedPic() const;

    bool isRealTime() const;
    bool isSeekable() const;
    bool seek(double seek_pos, double seek_incr);
	void setSeekType(SeekType type); 
	void setInterruptStatus(int interrupt);

//    const Packet& packet() const;
    int  readFrame();

    AVFormatContext *formatCtx() const;

    void setMediaInfo(MediaInfo *info);
    void initMediaInfo();

    int64_t startTime();
    int64_t duration();

    int stream() const;
    int streamIndex(MediaType type) const;
    AVStream* stream(MediaType type) const;
    const Packet& packet() const;

    double maxDuration() const;

private:
    DPTR_DECLARE(Demuxer)
};

NAMESPACE_END
#endif //SMP_DEMUXER_H
