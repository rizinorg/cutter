/** \file RizinCpp.h
 * Various utilities for easier and safer interactions with Rizin
 * from C++ code.
 */
#ifndef RIZINCPP_H
#define RIZINCPP_H

#include "rz_core.h"
#include <QString>
#include <memory>

static inline QString fromOwnedCharPtr(char *str)
{
    QString result(str ? str : "");
    rz_mem_free(str);
    return result;
}

template<class T, class F>
std::unique_ptr<T, F *> fromOwned(T *data, F *freeFunction)
{
    return std::unique_ptr<T, F *> { data, freeFunction };
}

static inline std::unique_ptr<char, decltype(&rz_mem_free)> fromOwned(char *text)
{
    return { text, rz_mem_free };
}

template<class T, void (*func)(T *)>
class FreeBinder
{
public:
    void operator()(T *data) { func(data); }
};

template<class T, void (*func)(T *)>
using UniquePtrC = std::unique_ptr<T, FreeBinder<T, func>>;

template<typename T, void (*func)(T)>
using UniquePtrCP = UniquePtrC<typename std::remove_pointer<T>::type, func>;

static inline auto fromOwned(RZ_OWN RzPVector *data)
        -> UniquePtrCP<decltype(data), &rz_pvector_free>
{
    return { data, {} };
}

static inline auto fromOwned(RZ_OWN RzList *data) -> UniquePtrCP<decltype(data), &rz_list_free>
{
    return { data, {} };
}

// Rizin list iteration macros
// deprecated, prefer using CutterPVector and CutterRzList instead
#define CutterRzListForeach(list, it, type, x)                                                     \
    if (list)                                                                                      \
        for (it = list->head; it && ((x = static_cast<type *>(it->val))); it = it->next)

#define CutterRzVectorForeach(vec, it, type)                                                       \
    if ((vec) && (vec)->a)                                                                         \
        for (it = (type *)(vec)->a;                                                                \
             (char *)it != (char *)(vec)->a + ((vec)->len * (vec)->elem_size);                     \
             it = (type *)((char *)it + (vec)->elem_size))

template<typename T>
class CutterPVector
{
private:
    const RzPVector *const vec;

public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T *;
        using difference_type = ptrdiff_t;
        using pointer = T **;
        using reference = T *&;

    private:
        T **p;

    public:
        iterator(T **p) : p(p) {}
        iterator(const iterator &o) : p(o.p) {}
        iterator &operator++()
        {
            p++;
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp(*this);
            operator++();
            return tmp;
        }
        bool operator==(const iterator &rhs) const { return p == rhs.p; }
        bool operator!=(const iterator &rhs) const { return p != rhs.p; }
        T *operator*() { return *p; }
    };

    CutterPVector(const RzPVector *vec) : vec(vec) {}
    iterator begin() const { return iterator(reinterpret_cast<T **>(vec->v.a)); }
    iterator end() const { return iterator(reinterpret_cast<T **>(vec->v.a) + vec->v.len); }
};

template<typename T>
class CutterRzList
{
private:
    const RzList *const list;

public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T *;
        using difference_type = ptrdiff_t;
        using pointer = T **;
        using reference = T *&;

    private:
        RzListIter *iter;

    public:
        explicit iterator(RzListIter *iter) : iter(iter) {}
        iterator(const iterator &o) : iter(o.iter) {}
        iterator &operator++()
        {
            if (!iter) {
                return *this;
            }
            iter = iter->next;
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp(*this);
            operator++();
            return tmp;
        }
        bool operator==(const iterator &rhs) const { return iter == rhs.iter; }
        bool operator!=(const iterator &rhs) const { return iter != rhs.iter; }
        T *operator*()
        {
            if (!iter) {
                return nullptr;
            }
            return reinterpret_cast<T *>(iter->val);
        }
    };

    explicit CutterRzList(const RzList *l) : list(l) {}
    iterator begin() const
    {
        if (!list) {
            return iterator(nullptr);
        }
        return iterator(list->head);
    }
    iterator end() const { return iterator(nullptr); }
};

template<typename T>
class CutterRzIter
{
    UniquePtrC<RzIterator, &rz_iterator_free> rzIter;

public:
    CutterRzIter(RzIterator *rzIter) : rzIter(rzIter)
    {
        // immediately attempt advancing by 1, otherwise it's hard to distinguish whether current
        // element is null due to not having called next, or due to having run out of elements
        if (rzIter) {
            ++*this;
        }
    }

    CutterRzIter<T> &operator++()
    {
        rz_iterator_next(rzIter.get());
        return *this;
    }
    operator bool() { return rzIter && rzIter->cur; }
    T &operator*() { return *reinterpret_cast<RzAnalysisBytes *>(rzIter->cur); }
    T *get() { return reinterpret_cast<RzAnalysisBytes *>(rzIter->cur); }
    T *operator->() { return reinterpret_cast<RzAnalysisBytes *>(rzIter->cur); }
};

#define CutterHtDef(xx, XX, K, VB)                                                                 \
    template<typename V>                                                                           \
    class CutterHt##XX                                                                             \
    {                                                                                              \
    private:                                                                                       \
        Ht##XX *const ht;                                                                          \
        template<typename F>                                                                       \
        static bool ForEachCb(void *user, K k, const VB v)                                         \
        {                                                                                          \
            return (*reinterpret_cast<F *>(user))(k, reinterpret_cast<const V *>(v));              \
        }                                                                                          \
                                                                                                   \
    public:                                                                                        \
        CutterHt##XX(Ht##XX *ht) : ht(ht) {};                                                      \
        template<typename F>                                                                       \
        void ForEach(F f)                                                                          \
        {                                                                                          \
            ht_##xx##_foreach(ht, ForEachCb<F>, &f);                                               \
        }                                                                                          \
    }

CutterHtDef(sp, SP, const char *, void *);

#endif // RIZINCPP_H
