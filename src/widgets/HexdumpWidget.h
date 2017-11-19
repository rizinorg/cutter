
#ifndef HEXDUMPWIDGET_H
#define HEXDUMPWIDGET_H

#include <array>
#include <QDebug>
#include <QTextEdit>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <memory>
#include "cutter.h"
#include "utils/Highlighter.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "Dashboard.h"


namespace Ui
{
    class HexdumpWidget;
}

class HexdumpWidget : public QDockWidget
{
Q_OBJECT

public:
    explicit HexdumpWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit HexdumpWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~HexdumpWidget();

    Highlighter        *highlighter;

signals:
    void fontChanged(QFont font);

public slots:
    void fillPlugins();

    QString normalize_addr(QString addr);

    QString normalizeAddr(QString addr);

    void selectHexPreview();

    void showOffsets(bool show);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void wheelEvent(QWheelEvent* event) override;

private:
	static const int linesMarginMin;
	static const int linesMarginDefault;
	static const int linesMarginMax;

    std::unique_ptr<Ui::HexdumpWidget> ui;

    RVA topOffset;
    RVA bottomOffset;

    void refresh(RVA addr = RVA_INVALID);
	void appendHexdumpLines(int lines, bool top);
	void removeHexdumpLines(int lines, bool top);

    void updateVisibleLines();
    std::array<QString, 3> fetchHexdump(RVA offset, RVA bytes);

	void connectScroll(bool disconnect);
    void setupScrollSync();

private slots:
    void on_seekChanged(RVA addr);
    void raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type);

    void highlightHexCurrentLine();
    void setFonts(QFont font);

    void highlightHexWords(const QString &str);
    void on_actionSettings_menu_1_triggered();
    void on_actionHideHexdump_side_panel_triggered();

    void showHexdumpContextMenu(const QPoint &pt);
    void showHexASCIIContextMenu(const QPoint &pt);

    void on_hexHexText_selectionChanged();
    void on_hexArchComboBox_2_currentTextChanged(const QString &arg1);
    void on_hexBitsComboBox_2_currentTextChanged(const QString &arg1);

    void on_action1column_triggered();
    void on_action2columns_triggered();
    void on_action4columns_triggered();
    void on_action8columns_triggered();
    void on_action16columns_triggered();
    void on_action32columns_triggered();
    void on_action64columns_triggered();

    void resizeHexdump();
    void adjustHexdumpLines();

    void on_codeCombo_2_currentTextChanged(const QString &arg1);
    void on_hexSideTab_2_currentChanged(int index);
    void on_memSideToolButton_clicked();
    void on_copyMD5_clicked();
    void on_copySHA1_clicked();
};

#endif // HEXDUMPWIDGET_H
