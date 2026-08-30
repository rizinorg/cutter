#ifndef HEXDIFFWIDGET_H
#define HEXDIFFWIDGET_H

#include "CutterDiffWindow.h"
#include "HexDiffView.h"

#include <QWidget>

#include <Configuration.h>
#include <CutterDiff.h>

namespace Ui {
class HexDiffWidget;
}

class HexDiffWidget : public CutterDiffWidget
{
    Q_OBJECT

public:
    explicit HexDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent);
    ~HexDiffWidget();
    void seek(QPair<RVA, RVA> addr);

private:
    Ui::HexDiffWidget *ui;
    HexDiffView *hexDiffView;
    void reload();
    void seekToDiffItem();

private:
    void onCopyMD5AClicked();
    void onCopyShA1AClicked();
    void onCopyShA256AClicked();
    void onCopyCrC32AClicked();
    void onCopyMD5BClicked();
    void onCopyShA1BClicked();
    void onCopyShA256BClicked();
    void onCopyCrC32BClicked();

    void clearParseWindow();
    void updateParseWindow(HexDiffView::Selection selection);
    void selectionChanged(HexDiffView::Selection selection);
};

#endif // HEXDIFFWIDGET_H
