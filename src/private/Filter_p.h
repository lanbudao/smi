#ifndef FILTER_P_H
#define FILTER_P_H

NAMESPACE_BEGIN

class FilterPrivate
{
public:
    FilterPrivate():
        enabled(true)
    {

    }
    virtual ~FilterPrivate()
    {

    }

    bool enabled;
};

class AudioFilterPrivate: public FilterPrivate
{
public:
    AudioFilterPrivate()
    {

    }
    virtual ~AudioFilterPrivate()
    {

    }
};

class VideoFilterPrivate: public FilterPrivate
{
public:
    VideoFilterPrivate()
    {

    }
    virtual ~VideoFilterPrivate()
    {

    }
};

NAMESPACE_END
#endif //FILTER_P_H
