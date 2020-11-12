#ifndef LIBAVFILTER_H
#define LIBAVFILTER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "Filter.h"

typedef struct AVFrame AVFrame;
typedef struct AVFilterContext AVFilterContext;
NAMESPACE_BEGIN

class Frame;
class AudioFrame;
class LibAVFilterPrivate;
class FFPRO_EXPORT LibAVFilter
{
public:
    explicit LibAVFilter();
    ~LibAVFilter();
    /*!
     * \brief The Status enum
     * Status of filter graph.
     * If setOptions() is called, the status is NotConfigured, and will to configure when processing
     * a new frame.
     * Filter graph will be reconfigured if options, incoming video frame format or size changed
     */
    enum Status {
        NotConfigured,
        ConfigureFailed,
        ConfigureOk
    };

    void setOptions(const std::string &opt);
    std::string options() const;

    Status status() const;

protected:
    virtual std::string sourceArguments() const = 0;
    bool putFrame(Frame* frame, bool changed, bool is_video = false);
    bool getFrame();
    void* getFrameHolder();

protected:
    LibAVFilterPrivate* priv;
};

class LibAVFilterAudioPrivate;
class FFPRO_EXPORT LibAVFilterAudio: public LibAVFilter, public AudioFilter
{
    DPTR_DECLARE_PRIVATE(LibAVFilterAudio)
public:
    LibAVFilterAudio();
    ~LibAVFilterAudio();

protected:
    bool process(MediaInfo *info, AudioFrame *frame = 0);
    std::string sourceArguments() const override;

};

class LibAVFilterVideoPrivate;
class FFPRO_EXPORT LibAVFilterVideo : public LibAVFilter, public VideoFilter
{
    DPTR_DECLARE_PRIVATE(LibAVFilterVideo)
public:
    LibAVFilterVideo();
    ~LibAVFilterVideo();

protected:
    bool process(MediaInfo *info, VideoFrame *frame = 0);
    std::string sourceArguments() const override;

};

NAMESPACE_END
#endif // LIBAVFILTER_H

