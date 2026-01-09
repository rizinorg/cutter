#ifndef TYPESVARIABLESDIALOG_H
#define TYPESVARIABLESDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include "CutterTreeWidget.h"

#include "core/Cutter.h"

namespace Ui {
class TypesVariablesDialog;
}

enum VariableScope { ALL = 0, GLOBAL, LOCAL };
/**
 * @brief Get a string representation of the given Scope.
 * @param scope The scope to convert.
 * @return A translated QString (e.g., "GLOBAL", "LOCAL").
 */
QString toString(VariableScope scope);

struct VariableEntry
{
    QString name;
    QString address;
    VariableScope scope;
    QString function; ///< Name of the function containing the variable (if local)
    RVA offset; ///< Offset for navigation (Function start for locals, absolute address for globals)
};

Q_DECLARE_METATYPE(VariableEntry)

class TypesVariablesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { NAME = 0, ADDRESS, SCOPE, FUNCTION, COUNT };
    static const int TypeVariableRole = Qt::UserRole;

    explicit TypesVariablesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void updateVariables(const QList<VariableEntry> &newVars);
    QList<VariableEntry> variables;
};

class TypesVariablesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TypesVariablesProxyModel(QObject *parent = nullptr);
    void setScope(VariableScope scope);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    VariableScope selectedScope = VariableScope::ALL;
};

class TypesVariablesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TypesVariablesDialog(QWidget *parent = nullptr, const QString &typeName = "");
    ~TypesVariablesDialog();

private slots:
    /**
     * @brief Navigate to the variable's location in the disassembly when an item is double-clicked.
     * @param index The index of the item clicked
     */
    void onItemDoubleClicked(const QModelIndex &index);

private:
    /**
     * @brief Populate the model with variables of the specified type by querying both globals and
     * local variables in all functions.
     * @param typeName The name of the type to filter by
     */
    void refreshModel(const QString &typeName);

    std::unique_ptr<Ui::TypesVariablesDialog> ui;
    TypesVariablesModel *sourceModel;
    TypesVariablesProxyModel *proxyModel;
    CutterTreeWidget *tree;
};

#endif // TYPESVARIABLESDIALOG_H
