#include "OutputSet.h"
#include "VideoRenderer.h"
#include <shared_mutex>

NAMESPACE_BEGIN

class OutputSetPrivate
{
public:
    OutputSetPrivate()
    {

    }

    ~OutputSetPrivate()
    {

    }

    std::list<AVOutput *> outputs;
    std::mutex mtx;
};

OutputSet::OutputSet():
    d_ptr(new OutputSetPrivate)
{

}

OutputSet::~OutputSet()
{

}

std::list<AVOutput *> OutputSet::outputs()
{
    DPTR_D(const OutputSet);
    return d->outputs;
}

//void OutputSet::initVideoRender()
//{
//	DPTR_D(OutputSet);
//	std::list<AVOutput *>::iterator itor;
//	for (itor = d->outputs.begin(); itor != d->outputs.end(); itor++) {
//		VideoRenderer *renderer = static_cast<VideoRenderer *>(*itor);
//		renderer->initVideoRender();
//	}
//}

void OutputSet::lock()
{
    DPTR_D(OutputSet);
    d->mtx.lock();
}

void OutputSet::unlock()
{
    DPTR_D(OutputSet);
    d->mtx.unlock();
}

void OutputSet::addOutput(AVOutput *output)
{
    DPTR_D(OutputSet);
    d->outputs.push_back(output);
}

void OutputSet::removeOutput(AVOutput *output)
{
    DPTR_D(OutputSet);
    d->outputs.remove(output);
}

void OutputSet::clearOutput()
{
    DPTR_D(OutputSet);
    std::list<AVOutput *>::iterator itor;
    for (itor = d->outputs.begin(); itor != d->outputs.end(); itor++) {
        AVOutput *output = static_cast<AVOutput *>(*itor);
        delete output;
    }
    d->outputs.clear();
}

void OutputSet::sendVideoFrame(const VideoFrame &frame)
{
    DPTR_D(OutputSet);
    std::list<AVOutput *>::iterator itor;
    for (itor = d->outputs.begin(); itor != d->outputs.end(); itor++) {
        VideoRenderer *renderer = static_cast<VideoRenderer *>(*itor);
        if (!renderer->isAvailable())
            continue;
        renderer->receive(frame);
    }
}

NAMESPACE_END
