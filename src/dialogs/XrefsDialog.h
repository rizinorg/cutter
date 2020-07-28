#ifndef XREFSDIALOG_H
#define XREFSDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <memory>
#include "common/Highlighter.h"
#include "core/Cutter.h"
#include "common/AddressableItemModel.h"

class XrefModel: public AddressableItemModel<QAbstractListModel>
{
private:
    QList<XrefDescription> xrefs;
    bool to;
public:
    enum Columns { OFFSET = 0, TYPE, CODE, COUNT };
    static const int FlagDescriptionRole = Qt::UserRole;

    XrefModel(QObject *parent = nullptr);
    void readForOffset(RVA offset, bool to, bool whole_function);
    void readForVariable(QString nameOfVariable, bool write, RVA offset);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;

    static QString xrefTypeString(const QString &type);
};


class MainWindow;

namespace Ui {
class XrefsDialog;
}

class XrefsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit XrefsDialog(MainWindow *main, QWidget *parent);
    ~XrefsDialog();

    void fillRefsForAddress(RVA addr, QString name, bool whole_function);
    /**
     * @brief Initializes toModel and fromModel with the details about the references to the specified
     * local variable 'nameOfVariable'.
     * @param nameOfVarible Name of the local variable for which the references are being initialized.
     * @param offset An offset in the function in which the specified local variable exist.
     */
    void fillRefsForVariable(QString nameOfVariable, RVA offset);

private slots:
    QString normalizeAddr(const QString &addr) const;

    void setupPreviewFont();
    void setupPreviewColors();

    void highlightCurrentLine();

    void onFromTreeWidgetItemSelectionChanged();
    void onToTreeWidgetItemSelectionChanged();

private:
    RVA addr;
    QString func_name;
    XrefModel toModel;
    XrefModel fromModel;

    std::unique_ptr<Ui::XrefsDialog> ui;

    void updateLabels(QString name);
    void updateLabelsForVariables(QString name);
    void updatePreview(RVA addr);
};

#endif // XREFSDIALOG_H
