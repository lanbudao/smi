#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include "sdk/global.h"
#include "utils/BlockQueue.h"
#include "AudioFrame.h"
#include "VideoFrame.h"
#include "subtitle/subtitleframe.h"

NAMESPACE_BEGIN

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9

class AudioFrameQueue: public BlockQueue<AudioFrame>
{
public:
    AudioFrameQueue()
    {
        setCapacity(SAMPLE_QUEUE_SIZE);
        setThreshold(SAMPLE_QUEUE_SIZE);
        blockFull(true);
    }
    ~AudioFrameQueue() PU_DECL_OVERRIDE {}
};

class VideoFrameQueue: public BlockQueue<VideoFrame>
{
public:
    VideoFrameQueue()
    {
        setCapacity(VIDEO_PICTURE_QUEUE_SIZE);
        setThreshold(VIDEO_PICTURE_QUEUE_SIZE);
        blockFull(true);
    }
    ~VideoFrameQueue() PU_DECL_OVERRIDE {}
};

class SubtitleFrameQueue : public BlockQueue<SubtitleFrame>
{
public:
    SubtitleFrameQueue()
    {
        setCapacity(SUBPICTURE_QUEUE_SIZE);
        setThreshold(SUBPICTURE_QUEUE_SIZE);
        blockFull(true);
    }
    ~SubtitleFrameQueue() PU_DECL_OVERRIDE {}
};

NAMESPACE_END
#endif // FRAMEQUEUE_H
