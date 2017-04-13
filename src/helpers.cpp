#include "helpers.h"

#include <QPlainTextEdit>
#include <QTextEdit>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QTreeWidget>
#include <QString>
#include <QAbstractItemView>


static QAbstractItemView::ScrollMode scrollMode()
{
    const bool use_scrollperpixel = true;
    return use_scrollperpixel ? QAbstractItemView::ScrollPerPixel : QAbstractItemView::ScrollPerItem;
}


namespace qhelpers
{
    // TODO: wouldn't it be enough to setFont on the QWidget?

    void normalizeFont(QPlainTextEdit *edit)
    {
#ifdef Q_OS_LINUX
        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
#endif
    }

    void normalizeEditFont(QTextEdit *edit)
    {
#ifdef Q_OS_LINUX
        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
#endif
    }

    QString uniqueProjectName(const QString &filename)
    {
        const QByteArray fullHash(QCryptographicHash::hash(filename.toUtf8(), QCryptographicHash::Sha1));
        return QFileInfo(filename).fileName() + "_" + fullHash.toHex().left(10);
    }

    void adjustColumns(QTreeWidget *tw)
    {
        int count = tw->columnCount();
        for (int i = 0; i != count; ++i)
        {
            tw->resizeColumnToContents(i);
        }
    }

    void appendRow(QTreeWidget *tw, const QString &str, const QString &str2,
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
    }

    void setVerticalScrollMode(QTreeWidget *tw)
    {
        tw->setVerticalScrollMode(scrollMode());
    }

} // end namespace
