#ifndef CONSTNAMEDIALOG_H
#define CONSTNAMEDIALOG_H

#include <QObject>
#include <QDialog>
#include <QShortcut>
#include <QListView>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QKeyEvent>
#include <QTextStream>
#include <memory>

namespace Ui {
class ConstNameDialog;
}

class ConstNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConstNameDialog(QWidget *parent = nullptr);
    ~ConstNameDialog();

    QString getConstName();
    void setConstValue(qulonglong constant);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

public slots:
    void on_doubleClicked(const QModelIndex &);

private:
    std::unique_ptr<Ui::ConstNameDialog> ui;
    static QMultiMap<qulonglong, QString> constMap;
    QStringListModel *model;
    QSortFilterProxyModel *proxyModel;    

    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // CONSTNAMEDIALOG_H
