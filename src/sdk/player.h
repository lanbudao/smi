#ifndef AVPLAYER_H
#define AVPLAYER_H

#include "global.h"
#include "DPTR.h"
#include "mediainfo.h"

/**
 *
 */
NAMESPACE_BEGIN

class Packet;
class Subtitle;
class AudioFilter;
class VideoFilter;
class RenderFilter;
class VideoRenderer;
class PlayerPrivate;
class SMI_EXPORT Player
{
    DPTR_DECLARE_PRIVATE(Player)
public:
    Player();
    ~Player();

	//void loadGLLoader(std::function<void*(const char*)> p);

    void setMedia(const std::string& url);
    int mediaStreamIndex(MediaType type);
    /**
     * @brief select desired audio stream, video stream and subtitle stream
     * Should be called before "prepare()" method
     * @param track
     */
    void setWantedStreamSpec(MediaType type, const char* spec);
    void setMediaStreamDisable(MediaType type);

    void prepare();
    /**
     * \brief prepare media asynchronously. 
     * It's usefull when load real-time streaming, such as rsp, rtsp, sdp...
     */
    void prepareAsync();

    MediaStatus mediaStatus();
    void pause(bool p = true);
    bool isPaused() const;
    void stop();
    /**
     * @brief set speed to play, the effective range is 0.5~2
     */
    void setSpeed(float speed);
    float speed() const;
    bool isMute() const;
    void setMute(bool m);
    /**
     * @brief set volume to play, the effective range is 0.0~1.0(default)
     */
    void setVolume(float volume);
    float volume() const;

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

    /**
     * @brief default is ResampleBase
     * @param t if is ResampleSoundtouch and speed don't equal 1.0, 
     * it will change speed without changing tone
     */
    void setResampleType(ResampleType t);

    void renderVideo();
    void setRenderCallback(std::function<void(void* vo_opaque)> cb);
    VideoRenderer* setVideoRenderer(int w, int h, void* opaque = nullptr);
    void addVideoRenderer(VideoRenderer *renderer);
    /**
     * @brief remove renderer. If param is null, remove all.
     */
    void removeRenderer(VideoRenderer *renderer = nullptr);
    void resizeWindow(int w, int h);

    /* Filter below */
    bool installFilter(AudioFilter *filter, int index = 0x7FFFFFFF);
    /**
     * @brief if render is NULL, the filter will apply to every videorender
     * You can install filter to the videorender appointed
     */
    bool installFilter(VideoFilter *filter, VideoRenderer* render = nullptr, int index = 0x7FFFFFFF);
    bool installFilter(RenderFilter *filter, VideoRenderer* render = nullptr, int index = 0x7FFFFFFF);

    /*
     *\brief Use 'setWantedStreamSpec' to switch the track of internal subtitle,
     * or 'addExternalSubtitle' to load external subtitle
     */
    void setInternalSubtitleEnabled(bool enabled, VideoRenderer* r = nullptr);
    Subtitle* addExternalSubtitle(const std::string &fileName, bool enabled = true, VideoRenderer* render = nullptr);
    void enableExternalSubtitle(Subtitle *sub);
    void removeExternalSubtitle(Subtitle *sub);
    std::list<Subtitle*> externalSubtitles();

    /* Call back */
    void setBufferProcessCallback(std::function<void(float p)> f);
    void setMediaStatusCallback(std::function<void(MediaStatus)> f);
    void setStreamChangedCallback(std::function<void(MediaType type, int stream)> f);

private:
    friend class SubtitleFilter;
    void setSubtitlePacketCallback(std::function<void(Packet* pkt)> f);
    void setSubtitleHeaderCallback(std::function<void(MediaInfo*)> f);
    void setSeekRequestCallback(std::function<void(double pos, double incr, SeekType type)> f);

private:
    class SMI_EXPORT DPTR_DECLARE(Player)
};

NAMESPACE_END
#endif //AVPLAYER_H
