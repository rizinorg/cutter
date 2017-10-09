#include "ImportsWidget.h"
#include "ui_ImportsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTreeWidget>
#include <QPen>
#include <QPainter>


void CMyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);

    itemOption.rect.adjust(10, 0, 0, 0);  // Make the item rectangle 10 pixels smaller from the left side.

    // Draw your item content.
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter, nullptr);

    // And now you can draw a bottom border.
    //painter->setPen(Qt::cyan);
    QPen pen = painter->pen();
    pen.setColor(Qt::white);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(itemOption.rect.bottomLeft(), itemOption.rect.bottomRight());
}

/*
 * Imports Widget
 */

ImportsWidget::ImportsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::ImportsWidget),
    main(main)
{
    ui->setupUi(this);

    // Delegate
    //CMyDelegate* delegate = new CMyDelegate(ui->importsTreeWidget);
    //ui->importsTreeWidget->setItemDelegate(delegate);

    ui->importsTreeWidget->hideColumn(0);
}

ImportsWidget::~ImportsWidget() {}

void ImportsWidget::setup()
{
    setScrollMode();

    fillImports();
}

void ImportsWidget::refresh()
{
    setup();
}

void ImportsWidget::fillImports()
{
    ui->importsTreeWidget->clear();
    for (auto i : CutterCore::getInstance()->getAllImports())
    {
        QTreeWidgetItem *item = qhelpers::appendRow(ui->importsTreeWidget, RAddressString(i.plt), i.type, "", i.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }

    highlightUnsafe();
    qhelpers::adjustColumns(ui->importsTreeWidget, 0, 10);
}

void ImportsWidget::highlightUnsafe()
{
    static const QString banned("[a-zA-Z_.]*(system|strcpy|strcpyA|strcpyW|wcscpy|_tcscpy|_mbscpy|StrCpy|StrCpyA|StrCpyW|lstrcpy|lstrcpyA|lstrcpyW" \
                                "|_tccpy|_mbccpy|_ftcscpy|strcat|strcatA|strcatW|wcscat|_tcscat|_mbscat|StrCat|StrCatA|StrCatW|lstrcat|lstrcatA|" \
                                "lstrcatW|StrCatBuff|StrCatBuffA|StrCatBuffW|StrCatChainW|_tccat|_mbccat|_ftcscat|sprintfW|sprintfA|wsprintf|wsprintfW|" \
                                "wsprintfA|sprintf|swprintf|_stprintf|wvsprintf|wvsprintfA|wvsprintfW|vsprintf|_vstprintf|vswprintf|strncpy|wcsncpy|" \
                                "_tcsncpy|_mbsncpy|_mbsnbcpy|StrCpyN|StrCpyNA|StrCpyNW|StrNCpy|strcpynA|StrNCpyA|StrNCpyW|lstrcpyn|lstrcpynA|lstrcpynW|" \
                                "strncat|wcsncat|_tcsncat|_mbsncat|_mbsnbcat|StrCatN|StrCatNA|StrCatNW|StrNCat|StrNCatA|StrNCatW|lstrncat|lstrcatnA|" \
                                "lstrcatnW|lstrcatn|gets|_getts|_gettws|IsBadWritePtr|IsBadHugeWritePtr|IsBadReadPtr|IsBadHugeReadPtr|IsBadCodePtr|" \
                                "IsBadStringPtr|memcpy|RtlCopyMemory|CopyMemory|wmemcpy|wnsprintf|wnsprintfA|wnsprintfW|_snwprintf|_snprintf|_sntprintf|" \
                                "_vsnprintf|vsnprintf|_vsnwprintf|_vsntprintf|wvnsprintf|wvnsprintfA|wvnsprintfW|strtok|_tcstok|wcstok|_mbstok|makepath|" \
                                "_tmakepath| _makepath|_wmakepath|_splitpath|_tsplitpath|_wsplitpath|scanf|wscanf|_tscanf|sscanf|swscanf|_stscanf|snscanf|" \
                                "snwscanf|_sntscanf|_itoa|_itow|_i64toa|_i64tow|_ui64toa|_ui64tot|_ui64tow|_ultoa|_ultot|_ultow|CharToOem|CharToOemA|CharToOemW|" \
                                "OemToChar|OemToCharA|OemToCharW|CharToOemBuffA|CharToOemBuffW|alloca|_alloca|strlen|wcslen|_mbslen|_mbstrlen|StrLen|lstrlen|" \
                                "ChangeWindowMessageFilter)");

    QList<QTreeWidgetItem *> clist = ui->importsTreeWidget->findItems(banned, Qt::MatchRegExp, 4);
    foreach (QTreeWidgetItem *item, clist)
    {
        item->setText(3, "Unsafe");
        //item->setBackgroundColor(4, QColor(255, 129, 123));
        //item->setForeground(4, Qt::white);
        item->setForeground(4, QColor(255, 129, 123));
    }
    //ui->importsTreeWidget->setStyleSheet("QTreeWidget::item { padding-left:10px; padding-top: 1px; padding-bottom: 1px; border-left: 10px; }");
}

void ImportsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->importsTreeWidget);
}

void ImportsWidget::on_importsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int /* column */)
{
    ImportDescription imp = item->data(0, Qt::UserRole).value<ImportDescription>();
    CutterCore::getInstance()->seek(imp.plt);
}
