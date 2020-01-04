#ifndef QHELPERS_H
#define QHELPERS_H

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
class QAbstractButton;
class QWidget;
class QTreeView;
class QAction;
class QMenu;
class QPaintDevice;
class QComboBox;

namespace qhelpers {
QString formatBytecount(const uint64_t bytecount);
void adjustColumns(QTreeView *tv, int columnCount, int padding);
void adjustColumns(QTreeWidget *tw, int padding);
bool selectFirstItem(QAbstractItemView *itemView);
QTreeWidgetItem *appendRow(QTreeWidget *tw, const QString &str, const QString &str2 = QString(),
                           const QString &str3 = QString(), const QString &str4 = QString(), const QString &str5 = QString());

void setVerticalScrollMode(QAbstractItemView *tw);

void setCheckedWithoutSignals(QAbstractButton *button, bool checked);


struct SizePolicyMinMax {
    QSizePolicy sizePolicy;
    int min;
    int max;

    void restoreWidth(QWidget *widget);
    void restoreHeight(QWidget *widget);
};

SizePolicyMinMax forceWidth(QWidget *widget, int width);
SizePolicyMinMax forceHeight(QWidget *widget, int height);

int getMaxFullyDisplayedLines(QTextEdit *textEdit);
int getMaxFullyDisplayedLines(QPlainTextEdit *plainTextEdit);

QByteArray applyColorToSvg(const QByteArray &data, QColor color);
QByteArray applyColorToSvg(const QString &filename, QColor color);

void setThemeIcons(QList<QPair<void*, QString>> supportedIconsNames, std::function<void(void *, const QIcon &)> setter);

void prependQAction(QAction *action, QMenu *menu);
qreal devicePixelRatio(const QPaintDevice *p);
/**
 * @brief Select comboBox item by value in Qt::UserRole.
 * @param comboBox
 * @param data - value to search in combobox item data
 * @param defaultIndex - item to select in case no match
 */
void selectIndexByData(QComboBox *comboBox, QVariant data, int defaultIndex = -1);

} // qhelpers

#endif // HELPERS_H
