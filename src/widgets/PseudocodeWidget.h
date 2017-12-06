#ifndef PSEUDOCODEWIDGET_H
#define PSEUDOCODEWIDGET_H

#include <QDockWidget>
#include "ui_PseudocodeWidget.h"
#include "cutter.h"

namespace Ui
{
    class PseudocodeWidget;
}

class PseudocodeWidget : public QDockWidget
{
Q_OBJECT

public:
    explicit PseudocodeWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit PseudocodeWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~PseudocodeWidget();

private:
    std::unique_ptr<Ui::PseudocodeWidget> ui;
    void refresh(RVA addr);
    void refreshPseudocode();
    void setupFonts();

signals:

public slots:
private slots:
    void on_seekChanged(RVA addr);
    void raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type);
    void fontsUpdated();
    void colorsUpdatedSlot();
};

#endif // PSEUDOCODEWIDGET_H
