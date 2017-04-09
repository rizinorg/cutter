#ifndef FLAGSWIDGET_H
#define FLAGSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>

class MainWindow;

namespace Ui
{
    class FlagsWidget;
}

class FlagsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FlagsWidget(MainWindow *main, QWidget *parent = 0);
    ~FlagsWidget();

    QTreeWidget    *flagsTreeWidget;
    QComboBox      *flagspaceCombo;

private slots:
    void on_flagsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_flagspaceCombo_currentTextChanged(const QString &arg1);

private:
    Ui::FlagsWidget *ui;

    MainWindow      *main;
};

#endif // FLAGSWIDGET_H
