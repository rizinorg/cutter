#ifndef YARA_DESCRIPTION_H
#define YARA_DESCRIPTION_H

#include <QMetaType>

struct YaraDescription
{
    RVA offset;
    RVA size;
    QString name;
};

Q_DECLARE_METATYPE(YaraDescription)

struct MetadataDescription
{
    QString name;
    QString value;
};

Q_DECLARE_METATYPE(MetadataDescription)

#endif // YARA_DESCRIPTION_H