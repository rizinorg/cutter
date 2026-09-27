#ifndef HEXDIFFWIDGET_H
#define HEXDIFFWIDGET_H

#include "CutterDiffWindow.h"
#include "HexDiffScrollBar.h"
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
    HexDiffScrollBar *addressScrollbar;
    HexDiffViewSyncer *syncer;
    HexDiffView *hexDiffViewA;
    HexDiffView *hexDiffViewB;
    void seekToDiffItem();

protected:
    void reload() override;

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
};

#endif // HEXDIFFWIDGET_H
