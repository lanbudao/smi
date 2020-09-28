#ifndef FACTORY_H
#define FACTORY_H

#include <ctype.h>
#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#ifndef _MSC_VER
#include <strings.h>
#endif

#include "Singleton.h"

#define FACTORY_REGISTER(BASE, _ID, NAME) FACTORY_REGISTER_ID_TYPE(BASE, BASE##Id_##_ID, BASE##_ID, NAME)
#define FACTORY_REGISTER_ID_TYPE(BASE, ID, TYPE, NAME) \
    FACTORY_REGISTER_ID_TYPE_AUTO(BASE, ID, TYPE, NAME) \
    bool Register##TYPE##_Man() { \
        return BASE::Register<TYPE>(ID, NAME); \
    }

#define FACTORY_REGISTER_ID_TYPE_AUTO(BASE, ID, TYPE, NAME) \
    namespace { \
        static const struct factory_register_##TYPE { \
            inline factory_register_##TYPE() { \
                BASE::Register<TYPE>(ID, NAME); \
            } \
        } sInit_##TYPE; \
    }

#define FACTORY_INTERFACE(T) \
    public: \
        template<class C> static bool Register(T##Id id, const char* name) { return Register(id, create<C>, name);} \
        static T* create(T##Id id); \
        static T* create(const char* name); \
        static T##Id* next(T##Id* id = nullptr); \
        static const char* name(T##Id id); \
        static T##Id id(const char* name); \
    private: \
        template<class C> static T* create() { return new C();} \
        typedef T* (*T##Creator)(); \
        static bool Register(T##Id id, T##Creator, const char *name);

#define FACTORY_DEFINE(T) \
    class T##Factory : public Factory<T##Id, T, T##Factory> {}; \
    bool T::Register(T##Id id, T##Creator c, const char *name) { \
        printf(#T "::Register(%s)\n", name); \
        return T##Factory::instance().registerCreator(id, c) && T##Factory::instance().registerIdName(id, name); \
    } \
    T* T::create(T##Id id) {return T##Factory::instance().create(id);} \
    T* T::create(const char* name) { return T::create(T::id(name));} \
    T##Id* T::next(T##Id *id) { \
        const std::vector<T##Id>& ids = T##Factory::instance().registeredIds(); \
        if (!id) return (T##Id*)&ids[0]; \
        T##Id *id0 = (T##Id*)&ids[0], *id1 = (T##Id*)&ids[ids.size() - 1]; \
        if (id >= id0 && id < id1) return id + 1; \
        if (id == id1) return nullptr; \
        std::vector<T##Id>::const_iterator it = std::find(ids.begin(), ids.end(), *id); \
        if (it == ids.end()) return nullptr; \
        return (T##Id*)&*(it++); \
    } \
    T##Id T::id(const char* name) { printf(#T "::id(\"%s\")\n", name); return T##Factory::instance().id(name, false);} \
    const char* T::name(T##Id id) {return T##Factory::instance().name(id);}

/**
 * Internal class
 */
template <typename Id, typename T, class Class>
class Factory : public Singleton<Class>
{
    DISABLE_COPY(Factory)
    typedef Id ID;
    typedef T Type;
    typedef Type* (*Creator)();
public:
    Type* create(const ID& id);
    bool registerCreator(const ID& id, const Creator& callback);
    bool registerIdName(const ID& id, const char* name);
    bool unregisterCreator(const ID& id);

    ID id(const char* name, bool caseSensitive = true) const;
    const char* name(const ID &id) const;
    size_t count() const;
    const std::vector<ID> &registeredIds() const;
    std::vector<const char*> registeredNames() const;

protected:
    Factory() {}
    virtual ~Factory() {}

private:
    template<class C>
    static Type* create() {
        return new C();
    }
    typedef std::map<ID, Creator> CreatorMap;
    CreatorMap creators;
    std::vector<ID> ids;
    typedef std::map<ID, const char*> NameMap;
    NameMap name_map; //static?
};

template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::Type *Factory<Id, T, Class>::create(const ID& id)
{
    typename CreatorMap::const_iterator it = creators.find(id);
    if (it == creators.end()) {
        printf("Unknown id\n");
        return 0;
        //throw std::runtime_error(err_msg.arg(id).toStdString());
    }
    return (it->second)();
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::registerCreator(const ID& id, const Creator& callback)
{
    //printf("%p id [%d] registered. size=%d\n", &Factory<Id, T, Class>::instance(), id, ids.size());
    ids.insert(ids.end(), id);
    return creators.insert(typename CreatorMap::value_type(id, callback)).second;
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::registerIdName(const ID& id, const char* name)
{
    return name_map.insert(typename NameMap::value_type(id, name/*.toLower()*/)).second;
}

template<typename Id, typename T, class Class>
bool Factory<Id, T, Class>::unregisterCreator(const ID& id)
{
    //printf("Id [%d] unregistered\n", id);
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    name_map.erase(id);
    return creators.erase(id) == 1;
}

template<typename Id, typename T, class Class>
typename Factory<Id, T, Class>::ID Factory<Id, T, Class>::id(const char* name, bool caseSensitive) const
{
#ifdef _MSC_VER
#define strcasecmp(s1, s2) _strcmpi(s1, s2)
#endif
    //need 'typename'  because 'Factory<Id, T, Class>::NameMap' is a dependent scope
    for (typename NameMap::const_iterator it = name_map.begin(); it!=name_map.end(); ++it) {
        if (caseSensitive) {
            if (it->second == name || !strcmp(it->second, name))
                return it->first;
        } else {
            if (!strcasecmp(it->second, name)) {
                return it->first;
            }
        }
    }
    printf("Not found\n");
    return ID(); //can not return ref. TODO: Use a ID wrapper class
}

template<typename Id, typename T, class Class>
const char* Factory<Id, T, Class>::name(const ID &id) const
{
    typename NameMap::const_iterator it = name_map.find(id);
    if (it == name_map.end())
        return nullptr;
    return it->second;
}

template<typename Id, typename T, class Class>
const std::vector<Id>& Factory<Id, T, Class>::registeredIds() const
{
    return ids;
}

template<typename Id, typename T, class Class>
std::vector<const char*> Factory<Id, T, Class>::registeredNames() const
{
    std::vector<const char*> names;
    for (typename NameMap::const_iterator it = name_map.begin(); it != name_map.end(); ++it) {
        names.push_back((*it).second);
    }
    return names;
}

template<typename Id, typename T, class Class>
size_t Factory<Id, T, Class>::count() const
{
    //printf("%p size = %d", &Factory<Id, T, Class>::instance(), ids.size());
    return ids.size();
}

#endif //FACTORY_H
