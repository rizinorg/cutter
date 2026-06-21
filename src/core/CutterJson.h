#ifndef CUTTER_JSON_H
#define CUTTER_JSON_H

#include "core/CutterCommon.h"

#include <QString>
#include <QStringList>

#include <memory>
#include <rz_project.h>
#include <utility>

class CutterJsonOwner;

/**
 * @brief Cutter wrapper for handling Json
 */
class CUTTER_EXPORT CutterJson
{
public:
    class Iterator
    {
    public:
        Iterator(const RzJson *value, std::shared_ptr<CutterJsonOwner> owner)
            : value(value), owner(std::move(owner))
        {
        }

        CutterJson operator*() const { return CutterJson(value, owner); }

        bool operator!=(const Iterator &other) const { return value != other.value; }

        Iterator &operator++()
        {
            value = value->next;
            return *this;
        }

    private:
        const RzJson *value;
        std::shared_ptr<CutterJsonOwner> owner;
    };

    CutterJson() : value(nullptr) {}

    CutterJson(const RzJson *value, std::shared_ptr<CutterJsonOwner> owner)
        : value(value), owner(std::move(owner))
    {
    }

    CutterJson first() const
    {
        return CutterJson(hasChildren() ? value->children.first : nullptr, owner);
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

    Iterator begin() const
    {
        return Iterator(hasChildren() ? value->children.first : nullptr, owner);
    }

    Iterator end() const { return Iterator(nullptr, nullptr); }

    bool toBool() const { return value && value->type == RZ_JSON_BOOLEAN && value->num.u_value; }
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
    size_t size() const { return hasChildren() ? value->children.count : 0; }
    RzJsonType type() const { return value ? value->type : RZ_JSON_NULL; }
    bool valid() const { return value ? true : false; }
    const RzJson *lowLevelValue() const { return value; }

private:
    bool hasChildren() const
    {
        return value && (value->type == RZ_JSON_OBJECT || value->type == RZ_JSON_ARRAY);
    }

    const RzJson *value;
    std::shared_ptr<CutterJsonOwner> owner;
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
