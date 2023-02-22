#include "JsonModel.h"

QTreeWidgetItem *Cutter::jsonTreeWidgetItem(const QString &key, const CutterJson &json)
{
    QString val;
    switch (json.type()) {
    case RZ_JSON_STRING:
        val = json.toString();
        break;
    case RZ_JSON_BOOLEAN:
        val = json.toBool() ? "true" : "false";
        break;
    case RZ_JSON_DOUBLE:
        val = QString::number(json.lowLevelValue()->num.dbl_value);
        break;
    case RZ_JSON_INTEGER:
        val = QString::number(json.toUt64());
        break;
    case RZ_JSON_NULL:
        val = "null";
        break;
    case RZ_JSON_OBJECT:
    case RZ_JSON_ARRAY:
        break;
    }
    auto r = new QTreeWidgetItem(QStringList({ key, val }));
    if (json.type() == RZ_JSON_ARRAY) {
        size_t i = 0;
        for (const auto &child : json) {
            r->addChild(jsonTreeWidgetItem(QString::number(i++), child));
        }
    } else if (json.type() == RZ_JSON_OBJECT) {
        for (const auto &child : json) {
            r->addChild(jsonTreeWidgetItem(child.key(), child));
        }
    }
    return r;
}
