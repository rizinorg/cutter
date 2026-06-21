#ifndef HEXDUMPWIDGET_H
#define HEXDUMPWIDGET_H

#include "Dashboard.h"
#include "HexWidget.h"
#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"

#include <QAction>
#include <QDebug>
#include <QMouseEvent>
#include <QTextEdit>

#include <memory>

namespace Ui {
class HexdumpWidget;
}

class RefreshDeferrer;
class QSyntaxHighlighter;

/**
 * @brief Hex dump widget containing a side panel for data parsing and hashing
 *
 * @see HexWidget
 */
class HexdumpWidget : public MemoryDockWidget
{
    Q_OBJECT
public:
    explicit HexdumpWidget(MainWindow *main);
    ~HexdumpWidget() override;

    static QString getWidgetType();

public slots:
    void initParsing();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    QWidget *widgetToFocusOnRaise() override;

private:
    std::unique_ptr<Ui::HexdumpWidget> ui;

    bool sentSeek = false;

    RefreshDeferrer *refreshDeferrer;
    QSyntaxHighlighter *syntaxHighLighter;

    void refresh();
    void refresh(RVA addr);
    void selectHexPreview();

    void setupFonts();

    void refreshSelectionInfo();
    void updateParseWindow(RVA start_address, int size);
    void clearParseWindow();
    void showSidePanel(bool show);

    QString getWindowTitle() const override;

private slots:
    void onSeekChanged(RVA addr);

    void selectionChanged(HexWidget::Selection selection);

    void onParseArchComboBoxCurrentTextChanged(const QString &arg1);
    void onParseBitsComboBoxCurrentTextChanged(const QString &arg1);
    void onParseTypeComboBoxCurrentTextChanged(const QString &arg1);
    void onParseEndianComboBoxCurrentTextChanged(const QString &arg1);

    void fontsUpdated();

    void onHexSideTab2CurrentChanged(int index);
    void onCopyMD5Clicked();
    void onCopyShA1Clicked();
    void onCopyShA256Clicked();
    void onCopyCrC32Clicked();
};

#endif // HEXDUMPWIDGET_H
