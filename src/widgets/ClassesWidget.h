#ifndef CLASSESWIDGET_H
#define CLASSESWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

namespace Ui {
class ClassesWidget;
}

class QTreeWidget;
class QTreeWidgetItem;
class MainWindow;
class ClassesWidget;

/**
 * @brief Common abstract base class for Bin and Anal classes models
 */
class ClassesModel : public AddressableItemModel<>
{
    Q_OBJECT
public:
    enum Columns { NAME = 0, REAL_NAME, TYPE, OFFSET, VTABLE, COUNT };

    /**
     * @brief values for TypeRole data
     */
    enum class RowType { Class = 0, Base, VTable, Method, Field };

    /**
     * @brief Offset role of data for QModelIndex
     *
     * will contain values of type RVA
     */
    static const int OffsetRole = Qt::UserRole;

    /**
     * @brief Name role of data for QModelIndex
     *
     * will contain values of QString, used for sorting,
     * as well as identifying classes and methods
     */
    static const int NameRole = Qt::UserRole + 1;

    /**
     * @brief Type role of data for QModelIndex
     *
     * will contain values of RowType
     */
    static const int TypeRole = Qt::UserRole + 2;

    /**
     * @brief VTable role of data for QModelIndex
     *
     * will contain values of type long long for sorting
     * by vtable offset
     */
    static const int VTableRole = Qt::UserRole + 3;

    /**
     * @brief Real Name role of data for QModelIndex
     *
     * will contain values of QString, used for sorting,
     * as well as identifying classes and methods
     */
    static const int RealNameRole = Qt::UserRole + 4;

    explicit ClassesModel(QObject *parent = nullptr) : AddressableItemModel(parent) {}

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

Q_DECLARE_METATYPE(ClassesModel::RowType)

class BinClassesModel : public ClassesModel
{
    Q_OBJECT

private:
    QList<BinClassDescription> classes;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

public:
    explicit BinClassesModel(QObject *parent = nullptr);
    void setClasses(const QList<BinClassDescription> &classes);
};

class AnalysisClassesModel : public ClassesModel
{
    Q_OBJECT

private:
    /**
     * @brief List entry below a class
     *
     * This roughly corresponds to attributes of Rizin analysis classes, which means it is not an
     * attribute in the sense of a class member variable, but any kind of sub-info associated with
     * the class. This struct in particular is used to provide a model for the list entries below a
     * class.
     */
    struct Attribute
    {
        enum class Type { VTable, Base, Method };
        Type type;
        QVariant data;

        Attribute() = default;
        Attribute(Type type, const QVariant &data) : type(type), data(data) {}
    };

    /**
     * This must always stay sorted alphabetically.
     */
    QList<QString> classes;

    RefreshDeferrer *refreshDeferrer;

    /**
     * @brief Cache for class attributes
     *
     * Maps class names to a list of Attributes.
     * This is filled only when the attributes of a specific class are requested.
     * (i.e. the user expands the class in the QTreeView)
     *
     * This must be a pointer instead of just a QMap, because it has to be modified
     * in methods that are defined as const by QAbstractItemModel.
     */
    std::unique_ptr<QMap<QString, QVector<Attribute>>> attrs;

    const QVector<Attribute> &getAttrs(const QString &cls) const;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

public:
    explicit AnalysisClassesModel(CutterDockWidget *parent);

public slots:
    void refreshAll();
    void classNew(const QString &cls);
    void classDeleted(const QString &cls);
    void classRenamed(const QString &oldName, const QString &newName);
    void classAttrsChanged(const QString &cls);
};

class ClassesSortFilterProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    explicit ClassesSortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
};

class ClassesWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit ClassesWidget(MainWindow *main);
    ~ClassesWidget();

private slots:
    void seekToVTableActionTriggered();
    void editMethodActionTriggered();
    void addMethodActionTriggered();
    void newClassActionTriggered();
    void renameClassActionTriggered();
    void deleteClassActionTriggered();

    void refreshClasses();
    void updateActions();

private:
    enum class Source { BIN, ANALYSIS };

    Source getSource();

    BinClassesModel *bin_model = nullptr;
    AnalysisClassesModel *analysis_model = nullptr;
    ClassesSortFilterProxyModel *proxy_model;

    QComboBox *classSourceCombo;

    QAction seekToVTableAction;
    QAction editMethodAction;
    QAction addMethodAction;
    QAction newClassAction;
    QAction renameClassAction;
    QAction deleteClassAction;
    QAction *classesMethodsSeparator;
};

#endif // CLASSESWIDGET_H
