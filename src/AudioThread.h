#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H

#include "AVThread.h"

NAMESPACE_BEGIN

class AudioThreadPrivate;
class AudioThread: public AVThread
{
    DPTR_DECLARE_PRIVATE(AudioThread)
public:
    AudioThread();
    virtual ~AudioThread() PU_DECL_OVERRIDE;

    void startDecode();
    void setDecoderThread(void* thread);

protected:
    void run() PU_DECL_OVERRIDE;

private:
};

NAMESPACE_END
#endif //AUDIOTHREAD_H
