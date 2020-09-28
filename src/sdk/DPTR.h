#ifndef DPTR_H
#define DPTR_H

#include <memory>

template <typename T> static inline T *GetPtrHelper(T *ptr) { return ptr; }

#define DPTR_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(GetPtrHelper(d_ptr.get())); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(GetPtrHelper(d_ptr.get())); } \
    friend class Class##Private;

#define DPTR_DECLARE_PRIVATE_D(Dptr, Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(Dptr.get()); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(Dptr.get()); } \
    friend class Class##Private;

#define DPTR_DECLARE_PUBLIC(Class) \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define DPTR_D(Class) Class##Private * const d = d_func()
#define DPTR_Q(Class) Class * const q = q_func()

#define DPTR_DECLARE(Class) \
    std::shared_ptr<Class##Private> d_ptr;

#endif //DPTR_H
