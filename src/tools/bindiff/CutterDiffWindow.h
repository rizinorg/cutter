#ifndef CUTTERDIFFWINDOW_H
#define CUTTERDIFFWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QSyntaxHighlighter>

#include <BinDiff.h>
#include <CutterDiff.h>

namespace Ui {
class CutterDiffWindow;
}

class HexDiffWidget;
class LineDiffWidget;
class GraphDiffWidget;
class FunctionNavWidget;
class DiffMatchWidget;
class DiffMisMatchWidget;
class FunctionDiffNavWidget;

class CutterDiffWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CutterDiffWindow(std::unique_ptr<CutterDiff> cutterDiff, QWidget *parent = nullptr);
    ~CutterDiffWindow();
    void showHexDiff();
    // I don't know how relevant is a seek feature for DiffedFiles
    // But it might be useful in cases where we have to compared
    // Current address in HexDiff to GraphDiff
    // We can highlight the block corresponding to the cursor address
    // in the graphdiff if not the particular line
    // An option to re-diff based on the transpose would also be good
    void seekAndShowHexDiff(QPair<RVA, RVA> addr);
    void showLineDiff();
    void showGraphDiff();
    void showMatches();
    void showRemoved();
    void showAdded();
public slots:
    void onActionDiffNewFile();
private slots:
    void showDiff();

private:
    Ui::CutterDiffWindow *ui;
    std::unique_ptr<CutterDiff> cutterDiff;

    FunctionDiffNavWidget *functionDiffNavWidget;
    HexDiffWidget *hexDiff = nullptr;
    LineDiffWidget *lineDiff = nullptr;
    GraphDiffWidget *graphDiff = nullptr;
    DiffMatchWidget *matchWidget = nullptr;
    DiffMisMatchWidget *addedWidget = nullptr;
    DiffMisMatchWidget *removedWidget = nullptr;

private:
    void addHexDiff();
    void addLineDiff();
    void addGraphDiff();
    void setupFonts();
    void exportDiff();
};

class CutterDiffWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CutterDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
        : QWidget(parent), cutterDiff(cutterDiff), diffWindow(parent)
    {
    }

protected:
    CutterDiff *cutterDiff;
    CutterDiffWindow *diffWindow;
};

#endif // CUTTERDIFFWINDOW_H
