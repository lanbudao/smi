#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "ByteArray.h"

NAMESPACE_BEGIN

class AudioFormatPrivate;
class FFPRO_EXPORT AudioFormat
{
    DPTR_DECLARE_PRIVATE(AudioFormat)

    enum {
        kSize = 12,
        kFloat = 1 << (kSize + 1),
        kUnsigned = 1 << (kSize + 2),
        kPlanar = 1 << (kSize + 3),
        kByteOther = 1 << (kSize + 4),
    };
public:
    AudioFormat();
    ~AudioFormat();
    AudioFormat(const AudioFormat &other);
    AudioFormat &operator=(const AudioFormat &other);
    bool operator==(const AudioFormat &other) const;
    /*!
     * \brief The SampleFormat enum
     * s8, u16, u24, s24, u32 are not listed in ffmpeg sample format and have not planar format.
     * pcm_s24le will be decoded as s32-24bit in ffmpeg, it's encoded as 32 bits, but raw sample has 24 bits
     */
    enum SampleFormat {
        SampleFormat_Unknown = 0,
        SampleFormat_Input = SampleFormat_Unknown,
        SampleFormat_Unsigned8 = 1 | kUnsigned,
        SampleFormat_Signed8 = 1,
        SampleFormat_Unigned16 = 2 | kUnsigned,
        SampleFormat_Signed16 = 2,
        SampleFormat_Unsigned24 = 3 | kUnsigned,
        SampleFormat_Signed24 = 3,
        SampleFormat_Unsigned32 = 4 | kUnsigned,
        SampleFormat_Signed32 = 4,
        SampleFormat_Float = 4 | kFloat,
        SampleFormat_Double = 8 | kFloat,
        SampleFormat_Unsigned8Planar = SampleFormat_Unsigned8 | kPlanar,
        SampleFormat_Signed16Planar = SampleFormat_Signed16 | kPlanar,
        SampleFormat_Signed32Planar = SampleFormat_Signed32 | kPlanar,
        SampleFormat_FloatPlanar = SampleFormat_Float | kPlanar,
        SampleFormat_DoublePlanar = SampleFormat_Double | kPlanar
    };
    enum ChannelLayout {
        ChannelLayout_Left,
        ChannelLayout_Right,
        ChannelLayout_Center,
        ChannelLayout_Mono = ChannelLayout_Center,
        ChannelLayout_Stereo,
        ChannelLayout_Unsupported //ok. now it's not complete
    };

    static const int64_t kHz = 1000000LL;

    inline bool rawSampleSize(SampleFormat format) {return format && ((1 << (kSize + 1)) - 1);}
    inline bool isFloat(SampleFormat format) {return !!(format & kFloat);}
    inline bool isPlanar(SampleFormat format) {return !!(format & kPlanar);}
    inline bool isUnsigned(SampleFormat format) {return !!(format & kUnsigned);}
    inline SampleFormat toPlanar(SampleFormat format) {return isPlanar(format) ? format : SampleFormat((int)format | kPlanar);}
    inline SampleFormat toPacked(SampleFormat format) {return isPlanar(format) ? SampleFormat((int)format ^ kPlanar) : format;}

    static AudioFormat::SampleFormat sampleFormatFromFFmpeg(int fffmt);
    static int sampleFormatToFFmpeg(AudioFormat::SampleFormat fmt);
    static ChannelLayout channelLayoutFromFFmpeg(int64_t clff);
    static int64_t channelLayoutToFFmpeg(ChannelLayout cl);

    bool isValid() const;
    bool isFloat() const;
    bool isPlanar() const;
    bool isUnsigned() const;

    int planeCount() const;

    int channels() const;
    void setChannels(int cls);

    int sampleRate() const;
    void setSampleRate(int rate);

    int bytesPerSample() const;
    int bytesPerFrame() const;
    int bytesPerSecond() const;

    int64_t durationForBytes(int64_t bytes);

    void setSampleFormat(SampleFormat sampleFormat);
    SampleFormat sampleFormat() const;
    void setSampleFormatFFmpeg(int ffSampleFormat);
    int sampleFormatFFmpeg() const;

    void setChannelLayoutFFmpeg(int64_t layout);
    int64_t channelLayoutFFmpeg() const;
    void setChannelLayout(ChannelLayout layout);
    ChannelLayout channelLayout() const;

    friend int RawSampleSize(SampleFormat fmt) { return fmt & ((1<<(kSize+1)) - 1); }
    friend bool IsFloat(SampleFormat fmt) { return !!(fmt & kFloat);}
    friend bool IsPlanar(SampleFormat format) { return !!(format & kPlanar);}
    friend bool IsUnsigned(SampleFormat format) {return !!(format & kUnsigned);}
    friend SampleFormat ToPlanar(SampleFormat fmt) { return IsPlanar(fmt) ? fmt : SampleFormat((int)fmt|kPlanar);}
    friend SampleFormat ToPacked(SampleFormat fmt) { return IsPlanar(fmt) ? SampleFormat((int)fmt^kPlanar) : fmt;}

    static AudioFormat::SampleFormat make(int bytesPerSample, bool isFloat, bool isUnsigned, bool isPlanar);

protected:
    DPTR_DECLARE(AudioFormat)
};

NAMESPACE_END
#endif //AUDIOFORMAT_H
