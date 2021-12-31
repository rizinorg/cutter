#ifndef QHELPERS_H
#define QHELPERS_H

#include "core/CutterCommon.h"

#include <QString>
#include <QColor>
#include <QSizePolicy>
#include <functional>

class QIcon;
class QPlainTextEdit;
class QTextEdit;
class QString;
class QTreeWidget;
class QTreeWidgetItem;
class QAbstractItemView;
class QAbstractItemModel;
class QAbstractButton;
class QWidget;
class QTreeView;
class QAction;
class QMenu;
class QPaintDevice;
class QComboBox;
class QSortFilterProxyModel;
class QMouseEvent;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#    define CUTTER_QT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
#else
#    define CUTTER_QT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#endif

namespace qhelpers {
CUTTER_EXPORT QString formatBytecount(const uint64_t bytecount);
CUTTER_EXPORT void adjustColumns(QTreeView *tv, int columnCount, int padding);
CUTTER_EXPORT void adjustColumns(QTreeWidget *tw, int padding);
CUTTER_EXPORT bool selectFirstItem(QAbstractItemView *itemView);
CUTTER_EXPORT QTreeWidgetItem *appendRow(QTreeWidget *tw, const QString &str,
                                         const QString &str2 = QString(),
                                         const QString &str3 = QString(),
                                         const QString &str4 = QString(),
                                         const QString &str5 = QString());

CUTTER_EXPORT void setVerticalScrollMode(QAbstractItemView *tw);

CUTTER_EXPORT void setCheckedWithoutSignals(QAbstractButton *button, bool checked);

struct CUTTER_EXPORT SizePolicyMinMax
{
    QSizePolicy sizePolicy;
    int min;
    int max;

    void restoreWidth(QWidget *widget);
    void restoreHeight(QWidget *widget);
};

CUTTER_EXPORT SizePolicyMinMax forceWidth(QWidget *widget, int width);
CUTTER_EXPORT SizePolicyMinMax forceHeight(QWidget *widget, int height);

CUTTER_EXPORT int getMaxFullyDisplayedLines(QTextEdit *textEdit);
CUTTER_EXPORT int getMaxFullyDisplayedLines(QPlainTextEdit *plainTextEdit);

CUTTER_EXPORT QByteArray applyColorToSvg(const QByteArray &data, QColor color);
CUTTER_EXPORT QByteArray applyColorToSvg(const QString &filename, QColor color);

CUTTER_EXPORT void setThemeIcons(QList<QPair<void *, QString>> supportedIconsNames,
                                 std::function<void(void *, const QIcon &)> setter);

CUTTER_EXPORT void prependQAction(QAction *action, QMenu *menu);
CUTTER_EXPORT qreal devicePixelRatio(const QPaintDevice *p);
/**
 * @brief Select comboBox item by value in Qt::UserRole.
 * @param comboBox
 * @param data - value to search in combobox item data
 * @param defaultIndex - item to select in case no match
 */
CUTTER_EXPORT void selectIndexByData(QComboBox *comboBox, QVariant data, int defaultIndex = -1);
/**
 * @brief Emit data change signal in a model's column (DisplayRole)
 * @param model - model containing data with changes
 * @param column - column in the model
 */
CUTTER_EXPORT void emitColumnChanged(QAbstractItemModel *model, int column);

CUTTER_EXPORT bool filterStringContains(const QString &string, const QSortFilterProxyModel *model);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using ColorFloat = float;
using KeyComb = QKeyCombination;
#else
using ColorFloat = qreal;
using KeyComb = int;
#endif

CUTTER_EXPORT QPointF mouseEventPos(QMouseEvent *ev);
CUTTER_EXPORT QPoint mouseEventGlobalPos(QMouseEvent *ev);

} // qhelpers

#endif // HELPERS_H
