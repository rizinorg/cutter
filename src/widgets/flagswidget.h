#ifndef FLAGSWIDGET_H
#define FLAGSWIDGET_H

#include "dockwidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class FlagsWidget;
}

class FlagsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit FlagsWidget(MainWindow *main, QWidget *parent = 0);
    ~FlagsWidget();

    void setup() override;

    void refresh() override;

    void clear();


private slots:
    void on_flagsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_flagspaceCombo_currentTextChanged(const QString &arg1);

private:
    Ui::FlagsWidget *ui;
    MainWindow      *main;

    void refreshFlags();
    void refreshFlagspaces();
    void setScrollMode();
};

#endif // FLAGSWIDGET_H
