#ifndef CTHREAD_H
#define CTHREAD_H

#include "sdk/DPTR.h"
#include "sdk/global.h"

NAMESPACE_BEGIN

class CThreadPrivate;
class CThread
{
    DPTR_DECLARE_PRIVATE(CThread)
public:
    CThread(const char* title = "");
    virtual ~CThread();

    void start();
    virtual void stop();
    void  wait();
    void sleep(int second);
    void msleep(int ms);
    bool isRunning() const;

    unsigned long id() const;

protected:
	/**
	 * @brief Inherit this class, and then override this function
	 */
    virtual void run();
    virtual void stoped();

private:
	DPTR_DECLARE(CThread);
};

NAMESPACE_END
#endif //CTHREAD_H
