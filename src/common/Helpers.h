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

namespace qhelpers {
QString formatBytecount(const long bytecount);
void adjustColumns(QTreeView *tv, int columnCount, int padding);
void adjustColumns(QTreeWidget *tw, int padding);
bool selectFirstItem(QTreeWidget* tw);
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

} // qhelpers

#endif // HELPERS_H
