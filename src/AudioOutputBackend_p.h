#ifndef AUDIO_OUTPUT_BACKEND_P_H
#define AUDIO_OUTPUT_BACKEND_P_H

#include "AudioFormat.h"

NAMESPACE_BEGIN

class AudioOutputBackendPrivate
{
public:
    AudioOutputBackendPrivate():
        avaliable(true)
    {

    }
    virtual ~AudioOutputBackendPrivate()
    {

    }

    AudioFormat format;
    bool avaliable;
};

NAMESPACE_END
#endif //AUDIO_OUTPUT_BACKEND_P_H
