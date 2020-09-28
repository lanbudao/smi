#ifndef AVPLAYER_H
#define AVPLAYER_H

#include "global.h"
#include "DPTR.h"
#include "SignalSlot.h"
#include "mediainfo.h"

/**
 *
 */
NAMESPACE_BEGIN

class Subtitle;
class AudioFilter;
class VideoFilter;
class VideoRenderer;
class PlayerPrivate;
class FFPRO_EXPORT Player
{
    DPTR_DECLARE_PRIVATE(Player)
public:
    Player();
    ~Player();

	void loadGLLoader(std::function<void*(const char*)> p);

    void setMedia(const char *url);
    /**
     * @brief select desired audio stream, video stream and subtitle stream
     * Should be called before "prepare()" method
     * @param track
     */
    void setWantedStreamSpec(MediaType type, const char* spec);

    void setMediaStreamDisable(MediaType type);

    void prepare();

    void pause(bool p = true);
    bool isPaused() const;
    void stop();
    /**
     * @brief set speed to play, The effective range is 0.5~2
     */
    void setSpeed(float speed);
    float speed() const;

    bool isPlaying();
    void seek(double t, SeekType type = SeekDefault);
    //void seek(double pos, double rel);
    void seekForward(int incr = 5);
    void seekBackward(int incr = -5);

    /**
     * @brief set the mode and value for buffer
     * @paras mode BufferPackets default
     * @paras value for BufferTime is 1000ms, for BufferBytes is 1024 Bytes, for BufferPackets is 24
     */
    void setBufferPara(BufferMode mode, int64_t value);

    MediaInfo* info();
    /**
     * @brief position
     * @return current pts of master clock, second
     */
    double position();
    /**
     * @brief return duration of this media, in
     * Seconds. Equals duration of mediainfo
     */
    int64_t duration();

    /**
     * @brief default is SyncToAudio if not set and audio stream is not null
     * Note: must call it before play() is called
     * @param type
     */
    void setClockType(ClockType type);

    void renderVideo();
    void setRenderCallback(std::function<void(void* vo_opaque)> cb);
    VideoRenderer* addVideoRenderer(int w, int h);
    void addVideoRenderer(VideoRenderer *renderer);
    void resizeWindow(int w, int h);

    void installFilter(AudioFilter *filter);
    void installFilter(VideoFilter *filter);

    std::map<std::string, std::string> internalSubtitles() const;
    Subtitle* internalSubtitle();
    Subtitle* addExternalSubtitle(const std::string &fileName, bool enabled = false);
    void enableExternalSubtitle(Subtitle *sub);
    void removeExternalSubtitle(Subtitle *sub);
    std::list<Subtitle*> externalSubtitles();

    /**
      interface
     */
    Signal<int> posChanged;
    void onPositionChanged(int);

    void setBufferProcessCallback(std::function<void(float p)> f);
    void setMediaStatusCallback(std::function<void(MediaStatus)> f);

private:

    DPTR_DECLARE(Player)
};

NAMESPACE_END
#endif //AVPLAYER_H
