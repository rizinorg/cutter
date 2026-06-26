#include "common/Helpers.h"

#include "Configuration.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDockWidget>
#include <QFileInfo>
#include <QMenu>
#include <QPlainTextEdit>
#include <QString>
#include <QTextEdit>
#include <QTreeWidget>
#include <QtCore>

#include <cmath>

namespace qhelpers {

QString formatByteCount(ut64 bytecount)
{
    if (bytecount == 0) {
        return "0";
    }

    const int exp = log(bytecount) / log(1024);
    constexpr char suffixes[] = { ' ', 'k', 'M', 'G', 'T', 'P', 'E' };

    QString str;
    QTextStream stream(&str);
    stream << qSetRealNumberPrecision(3) << bytecount / pow(1024, exp) << ' ' << suffixes[exp]
           << 'B';
    return stream.readAll();
}

void adjustColumns(QTreeView *tv, int columnCount, int padding)
{
    adjustColumns(tv, 0, columnCount, padding);
}

void adjustColumns(QTreeView *tv, int startIndex, int endIndex, int padding)
{
    if (!tv) {
        return;
    }

    for (int i = startIndex; i < endIndex; ++i) {
        tv->resizeColumnToContents(i);
        if (padding > 0) {
            const int width = tv->columnWidth(i);
            tv->setColumnWidth(i, width + padding);
        }
    }
}

void adjustColumns(QTreeWidget *tw, int padding)
{
    adjustColumns(tw, tw->columnCount(), padding);
}

void adjustColumn(QTreeView *tv, int columnIndex, int width)
{
    if (!tv) {
        return;
    }

    tv->resizeColumnToContents(columnIndex);
    if (width >= 0 && tv->columnWidth(columnIndex) > width) {
        tv->setColumnWidth(columnIndex, width);
    }
}

QTreeWidgetItem *appendRow(QTreeWidget *tw, const QString &str, const QString &str2,
                           const QString &str3, const QString &str4, const QString &str5)
{
    auto *tempItem = new QTreeWidgetItem();
    // Fill dummy hidden column
    tempItem->setText(0, "0");
    tempItem->setText(1, str);
    if (!str2.isNull()) {
        tempItem->setText(2, str2);
    }
    if (!str3.isNull()) {
        tempItem->setText(3, str3);
    }
    if (!str4.isNull()) {
        tempItem->setText(4, str4);
    }
    if (!str5.isNull()) {
        tempItem->setText(5, str5);
    }

    tw->insertTopLevelItem(0, tempItem);

    return tempItem;
}

bool selectFirstItem(QAbstractItemView *itemView)
{
    if (!itemView) {
        return false;
    }
    auto model = itemView->model();
    if (!model) {
        return false;
    }
    if (model->hasIndex(0, 0)) {
        itemView->setCurrentIndex(model->index(0, 0));
        return true;
    }
    return false;
}

void setVerticalScrollMode(QAbstractItemView *tw)
{
    tw->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void setCheckedWithoutSignals(QAbstractButton *button, bool checked)
{
    const bool blocked = button->signalsBlocked();
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

void SizePolicyMinMax::restoreWidth(QWidget *widget) const
{
    widget->setSizePolicy(sizePolicy.horizontalPolicy(), widget->sizePolicy().verticalPolicy());
    widget->setMinimumWidth(min);
    widget->setMaximumWidth(max);
}

void SizePolicyMinMax::restoreHeight(QWidget *widget) const
{
    widget->setSizePolicy(widget->sizePolicy().horizontalPolicy(), sizePolicy.verticalPolicy());
    widget->setMinimumHeight(min);
    widget->setMaximumHeight(max);
}

int getMaxFullyDisplayedLines(QTextEdit *textEdit)
{
    const QFontMetrics fontMetrics(textEdit->document()->defaultFont());
    return (textEdit->height()
            - (textEdit->contentsMargins().top() + textEdit->contentsMargins().bottom()
               + (int)(textEdit->document()->documentMargin() * 2)))
            / fontMetrics.lineSpacing();
}

int getMaxFullyDisplayedLines(QPlainTextEdit *plainTextEdit)
{
    const QFontMetrics fontMetrics(plainTextEdit->document()->defaultFont());
    return (plainTextEdit->height()
            - (plainTextEdit->contentsMargins().top() + plainTextEdit->contentsMargins().bottom()
               + (int)(plainTextEdit->document()->documentMargin() * 2)))
            / fontMetrics.lineSpacing();
}

QByteArray applyColorToSvg(const QByteArray &data, QColor color)
{
    static const QRegularExpression styleRegExp(
            "(?:style=\".*fill:(.*?);.*?\")|(?:fill=\"(.*?)\")");

    const QString replaceStr = QString("#%1").arg(color.rgb() & 0xffffff, 6, 16, QLatin1Char('0'));
    const int replaceStrLen = replaceStr.length();

    QString xml = QString::fromUtf8(data);

    int offset = 0;
    while (true) {
        const QRegularExpressionMatch match = styleRegExp.match(xml, offset);
        if (!match.hasMatch()) {
            break;
        }

        const int captureIndex = match.captured(1).isNull() ? 2 : 1;
        xml.replace(match.capturedStart(captureIndex), match.capturedLength(captureIndex),
                    replaceStr);
        offset = match.capturedStart(captureIndex) + replaceStrLen;
    }

    return xml.toUtf8();
}

QByteArray applyColorToSvg(const QString &filename, QColor color)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    return applyColorToSvg(file.readAll(), color);
}

void setThemeIcons(const QList<QPair<void *, QString>> &supportedIconsNames,
                   const std::function<void(void *, const QIcon &)> &setter)
{
    if (supportedIconsNames.isEmpty() || !setter) {
        return;
    }

    const QString &iconsDirPath = QStringLiteral(":/img/icons/");
    const QString &currTheme = Config()->getCurrentTheme()->name;
    const QString &relativeThemeDir = currTheme.toLower() + "/";
    QString iconPath;

    foreach (const auto &p, supportedIconsNames) {
        iconPath = iconsDirPath;
        // Verify that indeed there is an alternative icon in this theme
        // Otherwise, fallback to the regular icon folder
        if (QFile::exists(iconPath + relativeThemeDir + p.second)) {
            iconPath += relativeThemeDir;
        }
        setter(p.first, QIcon(iconPath + p.second));
    }
}

void prependQAction(QAction *action, QMenu *menu)
{
    auto actions = menu->actions();
    if (actions.empty()) {
        menu->addAction(action);
    } else {
        menu->insertAction(actions.first(), action);
    }
}

qreal devicePixelRatio(const QPaintDevice *p)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    return p->devicePixelRatioF();
#else
    return p->devicePixelRatio();
#endif
}

void selectIndexByData(QComboBox *widget, const QVariant &data, int defaultIndex)
{
    for (int i = 0; i < widget->count(); i++) {
        if (widget->itemData(i) == data) {
            widget->setCurrentIndex(i);
            return;
        }
    }
    widget->setCurrentIndex(defaultIndex);
}

void emitColumnChanged(QAbstractItemModel *model, int column)
{
    if (model && model->rowCount()) {
        emit model->dataChanged(model->index(0, column),
                                model->index(model->rowCount() - 1, column), { Qt::DisplayRole });
    }
}

bool filterStringContains(const QString &string, const QSortFilterProxyModel *model)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return string.contains(model->filterRegExp());
#else
    return string.contains(model->filterRegularExpression());
#endif
}

QPointF mouseEventPos(QMouseEvent *ev)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return ev->localPos();
#else
    return ev->position();
#endif
}

QPoint mouseEventGlobalPos(QMouseEvent *ev)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return ev->globalPos();
#else
    return ev->globalPosition().toPoint();
#endif
}

} // end namespace
