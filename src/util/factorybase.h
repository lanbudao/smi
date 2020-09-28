#ifndef FACTORYBASE_H
#define FACTORYBASE_H
#include <ctype.h>
#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#ifndef _MSC_VER
#include <strings.h>
#endif

/**
 * Internal class
 */
template <typename Id, typename T>
class FactoryBase
{
    typedef Id ID;
    typedef T Type;
    typedef Type* (*Creator)();
public:
    FactoryBase() {}
    virtual ~FactoryBase() {}

    Type* create(const ID& id)
    {
        typename CreatorMap::const_iterator it = creators.find(id);
        if (it == creators.end()) {
            printf("Unknown id\n");
            return 0;
            //throw std::runtime_error(err_msg.arg(id).toStdString());
        }
        return (it->second)();
    }

    bool registerCreator(const ID& id, const Creator& callback)
    {
        //printf("%p id [%d] registered. size=%d\n", &Factory<Id, T, Class>::instance(), id, ids.size());
        ids.insert(ids.end(), id);
        return creators.insert(typename CreatorMap::value_type(id, callback)).second;
    }

    bool registerIdName(const ID& id, const char* name)
    {
        return name_map.insert(typename NameMap::value_type(id, name/*.toLower()*/)).second;
    }

    bool unregisterCreator(const ID& id)
    {
        //printf("Id [%d] unregistered\n", id);
        ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
        name_map.erase(id);
        return creators.erase(id) == 1;
    }

    ID id(const char* name, bool caseSensitive = true) const;

    const char* name(const ID &id) const
    {
        typename NameMap::const_iterator it = name_map.find(id);
        if (it == name_map.end())
            return nullptr;
        return it->second;
    }

    size_t count() const {return ids.size();}
    const std::vector<ID> &registeredIds() const {return ids;}
    std::vector<const char*> registeredNames() const;

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

template<typename Id, typename T>
typename FactoryBase<Id, T>::ID FactoryBase<Id, T>::id(const char* name, bool caseSensitive) const
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

template<typename Id, typename T>
std::vector<const char*> FactoryBase<Id, T>::registeredNames() const
{
    std::vector<const char*> names;
    for (typename NameMap::const_iterator it = name_map.begin(); it != name_map.end(); ++it) {
        names.push_back((*it).second);
    }
    return names;
}

template<typename T, typename ID>
class FactoryInterface
{
public:
    FactoryInterface();
    ~FactoryInterface();
    static T* create(const char* name = "FFmpeg") {return f->create(id(name));}
    static T* create(ID id) {return f->create(id);}

    virtual ID id() const = 0;
    virtual std::string name() const;
    virtual std::string description() const;

    template<class C>
    static bool Register(FactoryBase<ID, T>* _f, ID id, const char *name) { return RegisterInternal(_f, id, create<C>, name); }

    /**
     * @brief next
     * @param id NULL to get the first id address
     * @return address of id or NULL if not found/end
     */
    static ID* next(ID* id = 0);
    static const char* name(ID id) {return f->name(id);}
    static ID id(const char* name)  {return f->id(name, false);}
private:
    template<class C>
    static T * create() { return new C(); }
    typedef T* (*VideoDecoderCreator)();

private:
    template<class Type>
    static bool RegisterInternal(FactoryBase<ID, T>* _f, ID id, VideoDecoderCreator c, const char *name)
    {
        f = _f;
        printf("::Register(%s)\n", name);
        return f->registerCreator(id, c) && f->registerIdName(id, name);
    }
    static FactoryBase<ID, T>* f;
};

template<typename T, typename ID>
ID* FactoryInterface<T, ID>::next(ID *id) {
    const std::vector<ID>& ids = f->registeredIds();
    if (!id) return &ids[0];
    ID *id0 = &ids[0], *id1 = &ids[ids.size() - 1];
    if (id >= id0 && id < id1) return id + 1;
    if (id == id1) return nullptr;
    typename std::vector<ID>::const_iterator it = std::find(ids.begin(), ids.end(), *id);
    if (it == ids.end()) return nullptr;
    return &(*(it++));
}

#endif // FACTORYBASE_H
