
#ifndef JSONMODEL_H
#define JSONMODEL_H

#include <QTreeWidgetItem>
#include "CutterJson.h"

namespace Cutter {

QTreeWidgetItem *jsonTreeWidgetItem(const QString &key, const CutterJson &json);

};

#endif // JSONMODEL_H
