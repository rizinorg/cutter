#ifndef PSEUDOCODEWIDGET_H
#define PSEUDOCODEWIDGET_H

#include <QDockWidget>
#include <memory>

#include "cutter.h"


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
    void on_seekChanged(RVA addr);
    void raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type);
    void fontsUpdated();
    void colorsUpdatedSlot();
    void refreshPseudocode();

private:
    void refresh(RVA addr);
    void setupFonts();

private:
    QTextEdit* textEditWidget;
    SyntaxHighlighter* syntaxHighLighter;
};

#endif // PSEUDOCODEWIDGET_H
