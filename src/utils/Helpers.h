#ifndef QHELPERS_H
#define QHELPERS_H

#include <QString>
#include <QColor>
#include <QSizePolicy>

class QPlainTextEdit;
class QTextEdit;
class QString;
class QTreeWidget;
class QTreeWidgetItem;
class QAbstractItemView;
class QAbstractButton;
class QWidget;

namespace qhelpers
{
    void adjustColumns(QTreeWidget *tw, int columnCount = 0, int padding = 0);

    QTreeWidgetItem *appendRow(QTreeWidget *tw, const QString &str, const QString &str2 = QString(),
                               const QString &str3 = QString(), const QString &str4 = QString(), const QString &str5 = QString());

    void setVerticalScrollMode(QAbstractItemView *tw);

    void setCheckedWithoutSignals(QAbstractButton *button, bool checked);


    struct SizePolicyMinMax
    {
        QSizePolicy sizePolicy;
        int min;
        int max;

        void restoreWidth(QWidget *widget);
        void restoreHeight(QWidget *widget);
    };

    SizePolicyMinMax forceWidth(QWidget *widget, int width);
    SizePolicyMinMax forceHeight(QWidget *widget, int height);


    int getMaxFullyDisplayedLines(QPlainTextEdit *plainTextEdit);

    QByteArray applyColorToSvg(const QByteArray &data, QColor color);
    QByteArray applyColorToSvg(const QString &filename, QColor color);
}

#endif // HELPERS_H
