#ifndef IMPORTSWIDGET_H
#define IMPORTSWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "common/AddressableItemModel.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractTableModel>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTreeWidgetItem>

class MainWindow;
class QTreeWidget;
class ImportsWidget;

/**
 * @brief Source model @ref ImportsWidget
 */
class ImportsModel : public AddressableItemModel<QAbstractTableModel>
{
    Q_OBJECT

    friend ImportsWidget;

private:
    const QRegularExpression banned = QRegularExpression(
            QStringLiteral("\\A(\\w\\.)*(system|strcpy|strcpyA|strcpyW|wcscpy|_tcscpy|_mbscpy|"
                           "StrCpy|StrCpyA|StrCpyW|lstrcpy|lstrcpyA|lstrcpyW"
                           "DCIEnum|DCIOpenProvider|DCISendCommand|DCIBeginAccess"
                           "|_tccpy|_mbccpy|_ftcscpy|strcat|strcatA|strcatW|wcscat|_tcscat|_mbscat|"
                           "StrCat|StrCatA|StrCatW|lstrcat|lstrcatA|"
                           "lstrcatW|StrCatBuff|StrCatBuffA|StrCatBuffW|StrCatChainW|_tccat|_"
                           "mbccat|_ftcscat|sprintfW|sprintfA|wsprintf|wsprintfW|"
                           "wsprintfA|sprintf|swprintf|_stprintf|wvsprintf|wvsprintfA|wvsprintfW|"
                           "vsprintf|_vstprintf|vswprintf|strncpy|wcsncpy|"
                           "_tcsncpy|_mbsncpy|_mbsnbcpy|StrCpyN|StrCpyNA|StrCpyNW|StrNCpy|strcpynA|"
                           "StrNCpyA|StrNCpyW|lstrcpyn|lstrcpynA|lstrcpynW|"
                           "strncat|wcsncat|_tcsncat|_mbsncat|_mbsnbcat|StrCatN|StrCatNA|StrCatNW|"
                           "StrNCat|StrNCatA|StrNCatW|lstrncat|lstrcatnA|"
                           "lstrcatnW|lstrcatn|gets|_getts|_gettws|IsBadWritePtr|IsBadHugeWritePtr|"
                           "IsBadReadPtr|IsBadHugeReadPtr|IsBadCodePtr|"
                           "IsBadStringPtr|memcpy|RtlCopyMemory|CopyMemory|wmemcpy|wnsprintf|"
                           "wnsprintfA|wnsprintfW|_snwprintf|_snprintf|_sntprintf|"
                           "_vsnprintf|vsnprintf|_vsnwprintf|_vsntprintf|wvnsprintf|wvnsprintfA|"
                           "wvnsprintfW|strtok|_tcstok|wcstok|_mbstok|makepath|"
                           "_tmakepath| "
                           "_makepath|_wmakepath|_splitpath|_tsplitpath|_wsplitpath|scanf|wscanf|_"
                           "tscanf|sscanf|swscanf|_stscanf|snscanf|"
                           "snwscanf|_sntscanf|_itoa|_itow|_i64toa|_i64tow|_ui64toa|_ui64tot|_"
                           "ui64tow|_ultoa|_ultot|_ultow|CharToOem|CharToOemA|CharToOemW|"
                           "OemToChar|OemToCharA|OemToCharW|CharToOemBuffA|CharToOemBuffW|alloca|_"
                           "alloca|strlen|wcslen|_mbslen|_mbstrlen|StrLen|lstrlen|"
                           "ChangeWindowMessageFilter|ChangeWindowMessageFilterEx)\\z"));
    QList<ImportDescription> imports;

public:
    enum Column : ut8 {
        AddressColumn = 0,
        TypeColumn,
        LibraryColumn,
        NameColumn,
        SafetyColumn,
        CommentColumn,
        ColumnCount
    };
    enum Role : ut16 { ImportDescriptionRole = Qt::UserRole, AddressRole };

    ImportsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
    QString libname(const QModelIndex &index) const;
    void reload();
};

/**
 * @brief Proxy model for @ref ImportsWidget
 */
class ImportsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    ImportsProxyModel(ImportsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widgets for visualizing imports
 */
class ImportsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit ImportsWidget(MainWindow *main);
    ~ImportsWidget();

private slots:
    void refreshImports();

private:
    ImportsModel *importsModel;
    ImportsProxyModel *importsProxyModel;

    void highlightUnsafe();
};

#endif // IMPORTSWIDGET_H
