#ifndef SDBWIDGET_H
#define SDBWIDGET_H

#include <memory>

#include "CutterDockWidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui {
class SdbWidget;
}

class SdbWidget : public CutterDockWidget
{
    Q_OBJECT

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
     Q_DISABLE_COPY_MOVE(SdbWidget)
#endif

public:
    explicit SdbWidget(MainWindow *main);

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    explicit SdbWidget(const SdbWidget& w) = delete;
    SdbWidget& operator=(const SdbWidget& w) = delete;
    explicit SdbWidget(SdbWidget&& w) = delete;
    SdbWidget operator=(SdbWidget&& w) = delete;
#endif

    ~SdbWidget() override;

private slots:
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_lockButton_clicked();
    void on_treeWidget_itemChanged(QTreeWidgetItem *item, int column);

    void reload(QString _path = QString());

private:
    std::unique_ptr<Ui::SdbWidget> ui;
    QString path;
};

#endif // SDBWIDGET_H
