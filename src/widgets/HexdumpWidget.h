#ifndef HEXDUMPWIDGET_H
#define HEXDUMPWIDGET_H

#include <QDebug>
#include <QTextEdit>
#include <QMouseEvent>
#include <QAction>

#include <array>
#include <memory>

#include "core/Cutter.h"
#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "common/Highlighter.h"
#include "common/HexAsciiHighlighter.h"
#include "common/HexHighlighter.h"
#include "common/SvgIconEngine.h"
#include "HexWidget.h"

#include "Dashboard.h"

namespace Ui {
class HexdumpWidget;
}

class RefreshDeferrer;

class HexdumpWidget : public MemoryDockWidget
{
    Q_OBJECT
public:
    explicit HexdumpWidget(MainWindow *main, QAction *action = nullptr);
    ~HexdumpWidget() override;
    Highlighter *highlighter;

public slots:
    void initParsing();

    void toggleSync();
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    QWidget *widgetToFocusOnRaise() override;
private:
    std::unique_ptr<Ui::HexdumpWidget> ui;

    bool sent_seek = false;

    RefreshDeferrer *refreshDeferrer;

    void refresh();
    void refresh(RVA addr);
    void selectHexPreview();

    void setupFonts();

    void refreshSelectionInfo();
    void updateParseWindow(RVA start_address, int size);
    void clearParseWindow();

    QAction syncAction;
    CutterSeekable *seekable;

private slots:
    void onSeekChanged(RVA addr);

    void on_actionHideHexdump_side_panel_triggered();

    void selectionChanged(HexWidget::Selection selection);

    void on_parseArchComboBox_currentTextChanged(const QString &arg1);
    void on_parseBitsComboBox_currentTextChanged(const QString &arg1);
    void on_parseTypeComboBox_currentTextChanged(const QString &arg1);
    void on_parseEndianComboBox_currentTextChanged(const QString &arg1);

    void fontsUpdated();

    void on_hexSideTab_2_currentChanged(int index);
    void on_copyMD5_clicked();
    void on_copySHA1_clicked();
};

#endif // HEXDUMPWIDGET_H
