#ifndef CUTTER_JSON_H
#define CUTTER_JSON_H

#include "core/CutterCommon.h"

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <rz_project.h>

class CutterJsonOwner;

class CUTTER_EXPORT CutterJson
{
public:
    class iterator
    {
    public:
        iterator(const RzJson *value, QSharedPointer<CutterJsonOwner> owner)
            : value(value), owner(owner)
        {
        }

        CutterJson operator*() const { return CutterJson(value, owner); }

        bool operator!=(const iterator &other) const { return value != other.value; }

        iterator &operator++()
        {
            value = value->next;
            return *this;
        }

    private:
        const RzJson *value;
        QSharedPointer<CutterJsonOwner> owner;
    };

    CutterJson() : value(nullptr), owner(nullptr) {}

    CutterJson(const RzJson *value, QSharedPointer<CutterJsonOwner> owner)
        : value(value), owner(owner)
    {
    }

    CutterJson first() const
    {
        return CutterJson(has_children() ? value->children.first : nullptr, owner);
    }

    CutterJson last() const;

    CutterJson operator[](const QString &key) const
    {
        QByteArray utf8 = key.toUtf8();
        return (*this)[utf8.data()];
    }

    CutterJson operator[](const char *key) const
    {
        return CutterJson(
                value && value->type == RZ_JSON_OBJECT ? rz_json_get(value, key) : nullptr, owner);
    }

    iterator begin() const
    {
        return iterator(has_children() ? value->children.first : nullptr, owner);
    }

    iterator end() const { return iterator(nullptr, nullptr); }

    bool toBool() const { return value && value->type == RZ_JSON_BOOLEAN && value->num.u_value; }
    QString toJson() const { return rz_json_as_string(value); }
    st64 toSt64() const { return value && value->type == RZ_JSON_INTEGER ? value->num.s_value : 0; }
    ut64 toUt64() const { return value && value->type == RZ_JSON_INTEGER ? value->num.u_value : 0; }

    RVA toRVA() const
    {
        return value && value->type == RZ_JSON_INTEGER ? value->num.u_value : RVA_INVALID;
    }

    QString toString() const
    {
        return value && value->type == RZ_JSON_STRING ? QString(value->str_value) : QString();
    }

    QString key() const { return value ? value->key : QString(); }
    QStringList keys() const;
    size_t size() const { return has_children() ? value->children.count : 0; }
    RzJsonType type() const { return value ? value->type : RZ_JSON_NULL; }
    bool valid() const { return value ? true : false; }
    const RzJson *lowLevelValue() const { return value; }

private:
    bool has_children() const
    {
        return value && (value->type == RZ_JSON_OBJECT || value->type == RZ_JSON_ARRAY);
    }

    const RzJson *value;
    QSharedPointer<CutterJsonOwner> owner;
};

class CUTTER_EXPORT CutterJsonOwner
{
public:
    CutterJsonOwner(RzJson *value, char *text) : value(value), text(text) {}

    virtual ~CutterJsonOwner()
    {
        rz_json_free(value);
        rz_mem_free(text);
    }

private:
    RzJson *value;
    char *text;
};

#endif // CUTTER_JSON_H
