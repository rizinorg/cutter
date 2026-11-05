#ifndef CUTTERDIFFWINDOW_H
#define CUTTERDIFFWINDOW_H

#include <QAction>
#include <QCloseEvent>
#include <QMainWindow>
#include <QSyntaxHighlighter>

#include <BinDiff.h>
#include <CutterDiff.h>
#include <deque>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    define CUTTER_FILTER_REGEXP() filterRegularExpression()
#else
#    define CUTTER_FILTER_REGEXP() filterRegExp()
#endif

namespace Ui {
class CutterDiffWindow;
}

enum TabWidgets : ut8 { MatchWidget, RemovedWidget, AddedWidget, HexDiff, LineDiff, GraphDiff };

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
    void showPrevMemoryWidget();
    void showDiffItemContextMenu(const QPoint &pos, int diffItemIndex, bool orig = true);

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
    struct DiffSeekLocation
    {
        int tabIndex;
        int diffItemIndex;

        bool operator==(const DiffSeekLocation &other) const
        {
            return tabIndex == other.tabIndex && diffItemIndex == other.diffItemIndex;
        }
    };

    std::deque<DiffSeekLocation> seekHistory;
    size_t currentSeekIndex = 0;
    bool seekingHistory = false;

    static constexpr size_t maxSeekHistory = 50;
    ut8 lastMemoryWidget = HexDiff;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void addHexDiff();
    void addLineDiff();
    void addGraphDiff();
    void setupFonts();
    void exportDiff();
    void seekHistoryForward();
    void seekHistoryBackward();
    void seekToCurrentHistory();
    void addSeekHistory();

    void updateSeekActions();
    void onTabIndexChanged();
signals:
    void reload();
};

class CutterDiffWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CutterDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent);

protected:
    CutterDiff *cutterDiff;
    CutterDiffWindow *diffWindow;
    virtual void reload() = 0;
};

#endif // CUTTERDIFFWINDOW_H
