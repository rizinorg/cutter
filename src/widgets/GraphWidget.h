#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "MemoryDockWidget.h"
#include <QLineEdit>

class MainWindow;
class DisassemblerGraphView;

class GraphWidget : public MemoryDockWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main, QAction *action = nullptr);
    ~GraphWidget() override {}

    DisassemblerGraphView *getGraphView() const;

    static QString getWidgetType();

signals:
    void graphClosed();

protected:
    QWidget *widgetToFocusOnRaise() override;

private:
    void closeEvent(QCloseEvent *event) override;

    QString getWindowTitle() const override;
    void prepareHeader();

    DisassemblerGraphView *graphView;
    QLineEdit *header = nullptr;
};

#endif // GRAPHWIDGET_H
