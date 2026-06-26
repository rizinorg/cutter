#ifndef XREFSDIALOG_H
#define XREFSDIALOG_H

#include "CutterCommon.h" // IWYU pragma: keep
#include "CutterDescriptions.h"
#include "common/AddressableItemModel.h"

#include <QDialog>
#include <QTreeWidgetItem>

#include <memory>

class XrefModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT
private:
    QList<XrefDescription> xrefs;
    bool to;

public:
    enum Columns : ut8 { OFFSET = 0, TYPE, CODE, COMMENT, COUNT };
    static const int flagDescriptionRole = Qt::UserRole;

    XrefModel(QObject *parent = nullptr);
    void readForOffset(RVA offset, bool to, bool whole_function);
    void readForVariable(const QString &nameOfVariable, bool write, RVA offset);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;

    static QString xrefTypeString(const QString &type);
    bool getTo() const;

    const XrefDescription *description(const QModelIndex &index) const;
};

class XrefFilterProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT
public:
    XrefFilterProxyModel(XrefModel *source_model, QObject *parent = nullptr);

private:
    bool to;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class MainWindow;

namespace Ui {
class XrefsDialog;
}

/**
 * @brief A dialog that displays X-refs to and from an address or variable, providing a live
 * disassembly preview
 */
class XrefsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit XrefsDialog(MainWindow *parent, bool hideXrefFrom = false);
    ~XrefsDialog();

    void fillRefsForAddress(RVA addr, const QString &name, bool whole_function);
    /**
     * @brief Initializes toModel and fromModel with the details about the references to the
     * specified local variable 'nameOfVariable'.
     * @param nameOfVarible Name of the local variable for which the references are being
     * initialized.
     * @param offset An offset in the function in which the specified local variable exist.
     */
    void fillRefsForVariable(const QString &nameOfVariable, RVA offset);

private slots:
    QString normalizeAddr(const QString &addr) const;

    void setupPreviewFont();
    void setupPreviewColors();

    void highlightCurrentLine();

    void onFromTreeWidgetItemSelectionChanged();
    void onToTreeWidgetItemSelectionChanged();

private:
    RVA addr;
    QString funcName;
    XrefModel toModel;
    XrefFilterProxyModel toProxyModel;
    XrefModel fromModel;
    XrefFilterProxyModel fromProxyModel;

    std::unique_ptr<Ui::XrefsDialog> ui;

    void updateLabels(const QString &name);
    void updateLabelsForVariables(const QString &name);
    void updatePreview(RVA addr);
    void hideXrefFromSection();
};

#endif // XREFSDIALOG_H
