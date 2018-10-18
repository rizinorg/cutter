#include "common/Helpers.h"

#include <cmath>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QFileInfo>
#include <QtCore>
#include <QCryptographicHash>
#include <QTreeWidget>
#include <QString>
#include <QAbstractItemView>
#include <QAbstractButton>
#include <QDockWidget>

static QAbstractItemView::ScrollMode scrollMode()
{
    const bool use_scrollperpixel = true;
    return use_scrollperpixel ? QAbstractItemView::ScrollPerPixel : QAbstractItemView::ScrollPerItem;
}


namespace qhelpers {

QString formatBytecount(const long bytecount)
{
    if (bytecount == 0)
        return "0";
    const int exp = log(bytecount) / log(1000);
    constexpr char suffixes[] = {' ', 'k', 'M', 'G', 'T', 'P', 'E'};

    QString str;
    QTextStream stream(&str);
    stream << qSetRealNumberPrecision(3) << bytecount / pow(1000, exp)
           << ' ' << suffixes[exp] << 'B';
    return stream.readAll();
}

void adjustColumns(QTreeView *tv, int columnCount, int padding)
{
    for (int i = 0; i != columnCount; ++i) {
        tv->resizeColumnToContents(i);
        if (padding > 0) {
            int width = tv->columnWidth(i);
            tv->setColumnWidth(i, width + padding);
        }
    }
}

void adjustColumns(QTreeWidget *tw, int padding)
{
    adjustColumns(tw, tw->columnCount(), padding);
}

QTreeWidgetItem *appendRow(QTreeWidget *tw, const QString &str, const QString &str2,
                           const QString &str3, const QString &str4, const QString &str5)
{
    QTreeWidgetItem *tempItem = new QTreeWidgetItem();
    // Fill dummy hidden column
    tempItem->setText(0, "0");
    tempItem->setText(1, str);
    if (!str2.isNull())
        tempItem->setText(2, str2);
    if (!str3.isNull())
        tempItem->setText(3, str3);
    if (!str4.isNull())
        tempItem->setText(4, str4);
    if (!str5.isNull())
        tempItem->setText(5, str5);

    tw->insertTopLevelItem(0, tempItem);

    return tempItem;
}

void setVerticalScrollMode(QAbstractItemView *tw)
{
    tw->setVerticalScrollMode(scrollMode());
}

void setCheckedWithoutSignals(QAbstractButton *button, bool checked)
{
    bool blocked = button->signalsBlocked();
    button->blockSignals(true);
    button->setChecked(checked);
    button->blockSignals(blocked);
}

SizePolicyMinMax forceWidth(QWidget *widget, int width)
{
    SizePolicyMinMax r;
    r.sizePolicy = widget->sizePolicy();
    r.min = widget->minimumWidth();
    r.max = widget->maximumWidth();

    QSizePolicy sizePolicy = r.sizePolicy;
    sizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
    widget->setSizePolicy(sizePolicy);
    widget->setMinimumWidth(width);
    widget->setMaximumWidth(width);

    return r;
}

SizePolicyMinMax forceHeight(QWidget *widget, int height)
{
    SizePolicyMinMax r;
    r.sizePolicy = widget->sizePolicy();
    r.min = widget->minimumHeight();
    r.max = widget->maximumHeight();

    QSizePolicy sizePolicy = r.sizePolicy;
    sizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    widget->setSizePolicy(sizePolicy);
    widget->setMinimumHeight(height);
    widget->setMaximumHeight(height);

    return r;
}

void SizePolicyMinMax::restoreWidth(QWidget *widget)
{
    widget->setSizePolicy(sizePolicy);
    widget->setMinimumWidth(min);
    widget->setMaximumWidth(max);
}

void SizePolicyMinMax::restoreHeight(QWidget *widget)
{
    widget->setSizePolicy(sizePolicy);
    widget->setMinimumHeight(min);
    widget->setMaximumHeight(max);
}

int getMaxFullyDisplayedLines(QTextEdit *textEdit)
{
    QFontMetrics fontMetrics(textEdit->document()->defaultFont());
    return (textEdit->height()
            - (textEdit->contentsMargins().top()
               + textEdit->contentsMargins().bottom()
               + (int)(textEdit->document()->documentMargin() * 2)))
           / fontMetrics.lineSpacing();
}

int getMaxFullyDisplayedLines(QPlainTextEdit *plainTextEdit)
{
    QFontMetrics fontMetrics(plainTextEdit->document()->defaultFont());
    return (plainTextEdit->height()
            - (plainTextEdit->contentsMargins().top()
               + plainTextEdit->contentsMargins().bottom()
               + (int)(plainTextEdit->document()->documentMargin() * 2)))
           / fontMetrics.lineSpacing();
}

QByteArray applyColorToSvg(const QByteArray &data, QColor color)
{
    static const QRegularExpression styleRegExp("(?:style=\".*fill:(.*?);.*?\")|(?:fill=\"(.*?)\")");

    QString replaceStr = QString("#%1").arg(color.rgb() & 0xffffff, 6, 16, QLatin1Char('0'));
    int replaceStrLen = replaceStr.length();

    QString xml = QString::fromUtf8(data);

    int offset = 0;
    while (true) {
        QRegularExpressionMatch match = styleRegExp.match(xml, offset);
        if (!match.hasMatch()) {
            break;
        }

        int captureIndex = match.captured(1).isNull() ? 2 : 1;
        xml.replace(match.capturedStart(captureIndex), match.capturedLength(captureIndex), replaceStr);
        offset = match.capturedStart(captureIndex) + replaceStrLen;
    }

    return xml.toUtf8();
}

QByteArray applyColorToSvg(const QString &filename, QColor color)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    return applyColorToSvg(file.readAll(), color);
}

} // end namespace
