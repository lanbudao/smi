#ifndef MEDIA_IO_H
#define MEDIA_IO_H

#include "DPTR.h"
#include "Factory.h"
#include "global.h"

NAMESPACE_BEGIN

typedef int MediaIOId;
class MediaIOPrivate;
class MediaIO
{
    FACTORY_INTERFACE(MediaIO)
    DPTR_DECLARE_PRIVATE(MediaIO)
public:
    virtual ~MediaIO();

    enum AccessMode {
        Read, // default
        Write
    };
    static std::list<std::string> supportProtocols();
    /*!
     * \brief createForProtocol
     * If an MediaIO subclass SomeInput.protocols() contains the protocol, return it's instance.
     * "QFile" input has protocols "qrc"(and empty "" means "qrc")
     * \return Null if none of registered MediaIO supports the protocol
     */
    static MediaIO* createForProtocol(const std::string& protocol);
    /*!
     * \brief createForUrl
     * Create a MediaIO and setUrl(url) if protocol of url is supported.
     * Example: MediaIO *qrc = MediaIO::createForUrl("qrc:/icon/test.mkv");
     * \return MediaIO instance with url set. Null if protocol is not supported.
     */
    static MediaIO* createForUrl(const std::string& url);
    virtual void setOptionsForIOCodec(const StringMap &) {};

    virtual std::string name() const = 0;
    /*!
     * \brief setUrl
     * onUrlChange() will be called if url is different. onUrlChange() will close the old url and open the new url if it's not empty
     * \param url
     */
    void setUrl(const std::string& url);
    std::string url() const;

    /// supported protocols. default is empty
    virtual const std::list<std::string>& protocols() const;
    virtual bool isSeekable() const = 0;
    virtual bool isWritable() const { return false; }
    /*!
     * \brief read
     * read at most maxSize bytes to data, and return the bytes were actually read
     */
    virtual int read(unsigned char *data, int64_t maxSize) = 0;
    /*!
     * \brief write
     * write at most maxSize bytes from data, and return the bytes were actually written
     */
    virtual int write(const unsigned char* data, int64_t maxSize) {
        PU_UNUSED(data);
        PU_UNUSED(maxSize);
        return 0;
    }
    /*!
     * \brief seek
     * \param from SEEK_SET, SEEK_CUR and SEEK_END from stdio.h
     * \return true if success
     */
    virtual bool seek(int64_t offset, int from = SEEK_SET) = 0;
    /*!
     * \brief position
     * MUST implement this. Used in seek
     * TODO: implement internally by default
     */
    virtual int64_t position() const = 0;
    /*!
     * \brief size
     * \return <=0 if not support
     */
    virtual int64_t size() const = 0;
    virtual int64_t clock() { return 0; };
    virtual int64_t startTimeUs() { return 0; };
    virtual int64_t duration() { return 0; };
    virtual void setDuration(int64_t duration) { (void*)duration; };
    /*!
     * \brief isVariableSize
     * Experiment: A hack for size() changes during playback.
     * If true, containers that estimate duration from pts(or bit rate) will get an invalid duration. Thus no eof get
     * when the size of playback start reaches. So playback will not stop.
     * Demuxer seeking should work for this case.
     */
    virtual bool isVariableSize() const { return false; }
    /*!
     * \brief setAccessMode
     * A MediaIO instance can be 1 mode, Read (default) or Write. If !isWritable(), then set to Write will fail and mode does not change
     * Call it before any function!
     * \return false if set failed
     */
    bool setAccessMode(AccessMode value);
    AccessMode accessMode() const;

    void setBufferSize(int value);
    int bufferSize() const;

    void* avioContext();
    void release();

protected:
    MediaIO(MediaIOPrivate *d = nullptr);
    DPTR_DECLARE(MediaIO)

    virtual void onUrlChanged() {}

};

NAMESPACE_END

#endif
