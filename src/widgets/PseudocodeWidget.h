#ifndef PSEUDOCODEWIDGET_H
#define PSEUDOCODEWIDGET_H

#include <QDockWidget>
#include <memory>

#include "cutter.h"

namespace Ui
{
    class PseudocodeWidget;
}

class QTextEdit;
class SyntaxHighlighter;

class PseudocodeWidget : public QDockWidget
{
Q_OBJECT

public:
    explicit PseudocodeWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit PseudocodeWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~PseudocodeWidget();

private slots:
    void raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type);
    void fontsUpdated();
    void colorsUpdatedSlot();
    void refreshPseudocode();

private:
    std::unique_ptr<Ui::PseudocodeWidget> ui;

    SyntaxHighlighter *syntaxHighLighter;

    void refresh(RVA addr);
    void setupFonts();
};

#endif // PSEUDOCODEWIDGET_H
