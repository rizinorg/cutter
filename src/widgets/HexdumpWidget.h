#ifndef HEXDUMPWIDGET_H
#define HEXDUMPWIDGET_H

#include <QDebug>
#include <QTextEdit>
#include <QMouseEvent>
#include <QAction>

#include <array>
#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "common/CutterSeekable.h"
#include "dialogs/HexdumpRangeDialog.h"
#include "common/Highlighter.h"
#include "common/HexAsciiHighlighter.h"
#include "common/HexHighlighter.h"
#include "common/SvgIconEngine.h"

#include "Dashboard.h"

namespace Ui {
    class HexdumpWidget;
}

class RefreshDeferrer;

class HexdumpWidget : public CutterDockWidget
{
    Q_OBJECT


public:
    explicit HexdumpWidget(MainWindow *main, QAction *action = nullptr);
    ~HexdumpWidget();
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

public slots:
    void initParsing();
    void on_rangeDialogAccepted();
    void showOffsets(bool show);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void toggleSync();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:
    static const int linesMarginMin = 32;
    static const int linesMarginDefault = 48;
    static const int linesMarginMax = 64;

    enum Format format = Format::Hex;

    std::unique_ptr<Ui::HexdumpWidget> ui;

    bool sent_seek = false;
    bool scroll_disabled = false;

    RVA first_loaded_address = RVA_INVALID;
    RVA last_loaded_address = RVA_INVALID;

    RefreshDeferrer *refreshDeferrer;

    void refresh(RVA addr = RVA_INVALID);
    void selectHexPreview();
    void updateHeaders();

    std::array<QString, 3> fetchHexdump(RVA addr, int lines);

    void connectScroll(bool disconnect_);
    void setupScrollSync();

    void setupFonts();

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

    void updateParseWindow(RVA start_address, int size);
    void clearParseWindow();

    int bufferLines = 0;
    int cols = 0;
    ut64 requestedSelectionStartAddress=0;
    ut64 requestedSelectionEndAddress=0;
    HexdumpRangeDialog  rangeDialog;
    QAction syncAction;
    CutterSeekable *seekable;

private slots:
    void onSeekChanged(RVA addr);
    void raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type);

    // Currently unused/untested
    // void highlightHexCurrentLine();
    // void highlightHexWords(const QString &str);

    void on_actionHideHexdump_side_panel_triggered();

    void showHexdumpContextMenu(const QPoint &pt);
    void showHexASCIIContextMenu(const QPoint &pt);

    void selectionChanged();
    void scrollChanged();

    void on_parseArchComboBox_currentTextChanged(const QString &arg1);
    void on_parseBitsComboBox_currentTextChanged(const QString &arg1);
    void on_parseTypeComboBox_currentTextChanged(const QString &arg1);
    void on_parseEndianComboBox_currentTextChanged(const QString &arg1);

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
    void colorsUpdatedSlot();

    void on_hexSideTab_2_currentChanged(int index);
    void on_copyMD5_clicked();
    void on_copySHA1_clicked();
};

#endif // HEXDUMPWIDGET_H
