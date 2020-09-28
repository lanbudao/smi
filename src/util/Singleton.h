#ifndef SINGLETON_H
#define SINGLETON_H

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "sdk/global.h"

/**
 * Internal class
 */

template <typename T>
class Singleton
{
    DISABLE_COPY(Singleton)
public:
    typedef T objectType;
    static T& instance();

protected:
    Singleton() {}
    virtual ~Singleton() {}

protected:
    static void destroy();
    static T* m_instance;
    static bool m_destroyed;
};

template<typename T>
T* Singleton<T>::m_instance = nullptr;

template<typename T>
bool Singleton<T>::m_destroyed = false;

template<typename T>
T &Singleton<T>::instance()
{
    if (!m_instance) {
        if (m_destroyed) {
            m_destroyed = false;
            exit(1);
        }
        m_instance = new T();
        std::atexit(&destroy);
    }
    return *m_instance;
}

template<typename T>
void Singleton<T>::destroy()
{
    assert(!m_destroyed);
    delete m_instance;
    m_instance = nullptr;
    m_destroyed = true;
}

#endif //SINGLETON_H
