#ifndef DVDIO_H
#define DVDIO_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "mediaio.h"

NAMESPACE_BEGIN

enum file_system {
    AUTO = 0,
    UDF,
    ISO9660
};

class DVDIOPrivate;
class DVDIO : public MediaIO
{
    DPTR_DECLARE_PRIVATE(DVDIO)
public:
    DVDIO();
    virtual std::string name() const;

    const std::list<std::string>& protocols() const
    {
        static std::list<std::string> p;
        p.push_back("dvd");
        return p;
    }

    virtual bool isSeekable() const override;
    virtual bool isWritable() const override;
    virtual int read(unsigned char *data, int64_t maxSize) override;
    virtual int write(const unsigned char *data, int64_t maxSize) override;
    virtual bool seek(int64_t offset, int from) override;
    virtual int64_t position() const override;
    /*!
    * \brief size
    * \return <=0 if not support
    */
    virtual int64_t size() const override;
    virtual int64_t clock() override;
    virtual int64_t startTimeUs() override;
    virtual int64_t duration() override;
    virtual void setDuration(int64_t duration) override;
    virtual void setOptionsForIOCodec(const StringMap & dict) override;

    /*£¡
     * \brief dvd interface
     */
    int setMouseActive(int x, int y);
    int setMouseSelect(int x, int y);
    int setUpperButtonSelect();
    int setLowerButtonSelect();
    int setLeftButtonSelect();
    int setRightButtonSelect();

protected:

    void onUrlChanged() override;

};

NAMESPACE_END

#endif