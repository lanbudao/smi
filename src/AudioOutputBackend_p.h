#ifndef AUDIO_OUTPUT_BACKEND_P_H
#define AUDIO_OUTPUT_BACKEND_P_H

#include "AudioFormat.h"

NAMESPACE_BEGIN

class AudioOutputBackendPrivate
{
public:
    AudioOutputBackendPrivate():
        avaliable(true),
        buffer_size(0),
        buffer_count(0)
    {

    }
    virtual ~AudioOutputBackendPrivate()
    {

    }

    AudioFormat format;
    bool avaliable;
    int buffer_size;
    int buffer_count;
};

NAMESPACE_END
#endif //AUDIO_OUTPUT_BACKEND_P_H
