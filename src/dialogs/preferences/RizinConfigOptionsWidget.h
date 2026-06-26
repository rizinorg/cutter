#ifndef RIZINCONFIGOPTIONSWIDGET_H
#define RIZINCONFIGOPTIONSWIDGET_H

#include "CutterDescriptions.h"
#include "PreferencesDialog.h"

#include <QAbstractItemModel>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStyledItemDelegate>

#include <memory>

namespace Ui {
class RizinConfigOptionsWidget;
}

/**
 * @brief Source model for @ref RizinConfigOptionsWidget
 */
class RizinConfigOptionsModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    QList<QString> categories;
    QMap<QString, QList<EvaluableVarDescription>> evalVarsMap;

public:
    static constexpr quintptr internalCategoryMarker = static_cast<quintptr>(-1);

    enum Column : ut8 { NameColumn = 0, ValueColumn, DescColumn, ColumnCount };
    enum Role : ut16 { EvaluableVarRole = Qt::UserRole + 1 };

    RizinConfigOptionsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    /**
     * @brief Makes top-level (spaces) items unselectable and boolean evaluable variables checkable
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /**
     * @brief Sets the updated value for evaluable variable in the UI
     */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void refresh();
    QList<QString> getCategories() const;
};

/**
 * @brief Proxy model for @ref RizinConfigOptionsWidget
 */
class RizinConfigOptionsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    RizinConfigOptionsProxyModel(RizinConfigOptionsModel *sourceModel, QObject *parent = nullptr);

    void setSelectedSpace(const QString &space);

protected:
    /**
     * @brief Accepts row if the NameColumn or DescColumn contain the searched text
     *
     * If the row is top-level row that contains only "evaluable variable space" name then it's only
     * accepted if any of its children items contain the searched text in NameColumn or DescColumn
     */
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString space;
};

/**
 * @brief Delegate for @ref RizinConfigOptionsWidget
 *
 * Used for rendering custom editor widgets when editing an evaluable variable
 *
 * @see IntervalDialog , StringListDialog
 */
class RizinConfigOptionsDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    /**
     * @brief Creates custom editor widgets for specific evaluable variables
     *
     * @return A combo-box containing all of the options that an evaluable variable can take.
     * If the evaluable variable can take any string input (contains no options) then the normal
     * string editor is shown.
     * In case of evaluable variables that take bool values it returns nullptr
     *
     * @see RizinConfigOptionsModel::flags() and RizinConfigOptionsModel::setData()
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    /**
     * @brief Selects the correct entry inside optiosn combo-box
     *
     * @see RizinOptionDelegate::createEditor()
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    /**
     * @brief Updates UI with data set from combo-box by calling @ref
     * RizinConfigOptionsModel::setData() with the current combo-box selected text as argument
     *
     * @see RizinOptionDelegate::createEditor()
     */
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

/**
 * @brief Widget for querying and modifying all evaluable variables available through rizin
 */
class RizinConfigOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit RizinConfigOptionsWidget(PreferencesDialog *parent = nullptr);
    ~RizinConfigOptionsWidget();

private:
    std::unique_ptr<Ui::RizinConfigOptionsWidget> ui;

    RizinConfigOptionsModel *sourceModel;
    RizinConfigOptionsProxyModel *proxyModel;
    RizinConfigOptionsDelegate *delegate;

    QMap<QString, QVariant> originalValues;

    enum OptionChanged : ut8 {
        Asm = 1 << 0,
        Debug = 1 << 1,
        Analysis = 1 << 2,
        Symbols = 1 << 3,

        All = Asm | Debug | Analysis | Symbols,

        Invalid = 1 << 4
    };

    void refresh();
    void refreshComboQuickFilter();
    void updateItemCount();
    void resetToDefaultBtnPressed();
    void restoreBtnPressed();
    void handleConfigOptionChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                   const QVector<int> &roles);
    OptionChanged getOptionChangedFromEvalVar(const QString &name);
    void triggerOptionsChanged(int option) const;
};

#endif // RIZINCONFIGOPTIONSWIDGET_H
