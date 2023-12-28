#ifndef OUTPUTSET_H
#define OUTPUTSET_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "VideoFrame.h"
#include "subtitle/subtitleframe.h"

NAMESPACE_BEGIN

class AVOutput;
class OutputSetPrivate;
class OutputSet
{
    DPTR_DECLARE_PRIVATE(OutputSet)
public:
    OutputSet();
    ~OutputSet();

    std::list<AVOutput *> outputs();

	//void initVideoRender();
    void lock();
    void unlock();

    void addOutput(AVOutput *output);
    void removeOutput(AVOutput *output);
    void clearOutput();

    void sendVideoFrame(const VideoFrame &frame);
    void sendSubtitleFrame(SubtitleFrame &frame);

private:
    DPTR_DECLARE(OutputSet)
};

NAMESPACE_END
#endif //OUTPUTSET_H
