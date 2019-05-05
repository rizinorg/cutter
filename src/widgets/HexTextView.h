#ifndef HexTextView_H
#define HexTextView_H

#include <QDebug>
#include <QTextEdit>
#include <QMouseEvent>
#include <QAction>
#include <QScrollArea>

#include <array>
#include <memory>

#include "core/Cutter.h"
#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "dialogs/HexdumpRangeDialog.h"
#include "common/Highlighter.h"
#include "common/HexAsciiHighlighter.h"
#include "common/HexHighlighter.h"
#include "common/SvgIconEngine.h"

#include "Dashboard.h"

namespace Ui {
    class HexTextView;
}

class RefreshDeferrer;

class HexTextView : public QScrollArea
{
    Q_OBJECT
public:
    explicit HexTextView(QWidget *parent);
    ~HexTextView();
    Highlighter        *highlighter;
    enum Format {
        Hex,
        Octal,
        // TODO:
//        HalfWord,
//        Word,
//        QuadWord,
//        Emoji,
//        SignedInt1,
//        SignedInt2,
//        SignedInt4,
    };
    void refresh(RVA addr = RVA_INVALID);
    void setupFonts();

    struct Selection {
        bool empty;
        RVA startAddress;
        RVA endAddress;
    };

    Selection getSelection();
    RVA position();
public slots:
    void on_rangeDialogAccepted();
    void showOffsets(bool show);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void zoomReset();
signals:
    void selectionChanged(Selection selection);
    void positionChanged(RVA start);
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:
    enum Format format = Format::Hex;

    std::unique_ptr<Ui::HexTextView> ui;

    bool sent_seek = false;
    bool scroll_disabled = false;

    RVA first_loaded_address = RVA_INVALID;
    RVA last_loaded_address = RVA_INVALID;

    void updateHeaders();

    std::array<QString, 3> fetchHexdump(RVA addr, int lines);

    void connectScroll(bool disconnect_);
    void setupScrollSync();



    // If bottom = false gets the FIRST displayed line, otherwise the LAST displayed
    // line.
    int getDisplayedLined(QTextEdit *textEdit, bool bottom = false);

    static void removeTopLinesWithoutScroll(QTextEdit *textEdit, int lines);
    static void removeBottomLinesWithoutScroll(QTextEdit *textEdit, int lines);
    static void prependWithoutScroll(QTextEdit *textEdit, QString text);
    static void appendWithoutScroll(QTextEdit *textEdit, QString text);
    static void setTextEditPosition(QTextEdit *textEdit, int position);

    RVA hexPositionToAddress(int position);
    RVA asciiPositionToAddress(int position);
    int hexAddressToPosition(RVA address);
    int asciiAddressToPosition(RVA address);
    void updateWidths();
    void syncScale();

    void updateParseWindow(RVA start_address, int size);
    void clearParseWindow();

    int bufferLines = 0;
    int cols = 0;
    ut64 requestedSelectionStartAddress=0;
    ut64 requestedSelectionEndAddress=0;
    HexdumpRangeDialog  rangeDialog;
    RVA currentPos = 0;
    qreal defaultFontSize;
    Selection selection;

private slots:
    // Currently unused/untested
    // void highlightHexCurrentLine();
    // void highlightHexWords(const QString &str);

    void showHexdumpContextMenu(const QPoint &pt);
    void showHexASCIIContextMenu(const QPoint &pt);

    void onSelectionChanged();
    void scrollChanged();

    void on_actionCopyAddressAtCursor_triggered();

    void on_action1column_triggered();
    void on_action2columns_triggered();
    void on_action4columns_triggered();
    void on_action8columns_triggered();
    void on_action16columns_triggered();
    void on_action32columns_triggered();
    void on_action64columns_triggered();

    void on_actionFormatHex_triggered();
    void on_actionFormatOctal_triggered();

    void on_actionSelect_Block_triggered();

    void fontsUpdated();
};

#endif // HexTextView_H
