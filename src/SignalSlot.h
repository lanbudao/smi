#ifndef SIGNALSLOT_H
#define SIGNALSLOT_H

#include "sdk/global.h"
#include <algorithm>
#include <iostream>
#include <vector>

NAMESPACE_BEGIN

template<typename ... Args>
class FFPRO_EXPORT SlotBase
{
public:
    virtual void exec(Args... args) = 0;
    virtual ~SlotBase() {}
};

template<class T, typename ... Args>
class FFPRO_EXPORT Slot : public SlotBase<Args...>
{
public:
    Slot(T* pObj, void (T::*func)(Args...)) {
        obj = pObj;
        mfunc = func;
    }
    void exec(Args... args) {
        (obj->*mfunc)(args...);
    }

private:
    T* obj;
    void (T::*mfunc)(Args...);
};

template<typename ... Args>
class FFPRO_EXPORT Signal
{
public:
    template<class T>
    void Bind(T* pObj, void (T::*func)(Args...)) {
        slotSet.push_back( new Slot<T,Args...>(pObj, func) );
    }
    void Bind(Signal* sig)
    {
        sigSet.push_back(sig);
    }
    template<class T>
    void Unbind(T *obj) {
        typename std::list<SlotBase<Args...>*>::iterator itor;
        for (itor = slotSet.begin(); itor != slotSet.end(); itor++) {
            if ((*itor) == obj) {
                delete obj;
                slotSet.remove(obj);
                return;
            }
        }
    }
    ~Signal() {
        typename std::list<SlotBase<Args...>*>::iterator itor;
        for (itor = slotSet.begin(); itor != slotSet.end(); itor++) {
            delete (*itor);
        }
        slotSet.clear();
        sigSet.clear();
    }
    void operator()(Args ...args) {
        typename std::list<SlotBase<Args...>*>::iterator itor;
        for (itor = slotSet.begin(); itor != slotSet.end(); itor++) {
            (*itor)->exec(args...);
        }
        typename std::list<Signal*>::iterator itSig;
        for (itSig = sigSet.begin(); itSig != sigSet.end(); itSig++) {
            (*(*itSig))(args...);
        }
    }
private:
    std::list<SlotBase<Args...>*> slotSet;
    std::list<Signal*> sigSet;
};

#define PU_LINK_SIG2SIG(sender, receiver) \
    (sender).Bind(&receiver);

//#define PU_LINK(sender, signal, receiver, method) \
//    ((sender)->signal.Bind(receiver, method))

#define PU_LINK(sender, signal, receiver, method) \
    ((sender)->signal.Bind(receiver, method))

#define PU_UNLINK(sender, signal, receiver) ((sender)->signal.Unbind(receiver))

NAMESPACE_END
#endif //SIGNALSLOT_H
