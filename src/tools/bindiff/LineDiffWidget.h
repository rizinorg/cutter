#ifndef LINEDIFFWIDGET_H
#define LINEDIFFWIDGET_H

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextBlock>
#include <QWidget>

#include <CutterDiff.h>

class DiffTextEdit;
class LineNumberArea;

class LineDiffWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineDiffWidget(CutterDiff *cutterDiff, QWidget *parent = nullptr);
    ~LineDiffWidget();
    void fetchFunctionDisasSplit(const CutterDiffItem &diffItem);
    void balanceLines();
    void setUpFonts();

protected:
private slots:
    void onViewModeChanged();

private:
    CutterDiff *cutterDiff;
    DiffTextEdit *leftEdit;
    DiffTextEdit *rightEdit;
    DiffTextEdit *unifiedEdit;
    QComboBox *viewSelector;
    QSplitter *splitViewSplitter;
    bool splitHorizontal = false;
    QPushButton *splitOrientationButton = nullptr;
    QLabel *functionLabel;
};

class DiffTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DiffTextEdit(QWidget *parent = nullptr);
    ~DiffTextEdit();
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void insertFormatted(const QString &text, const QColor &color = QColor());
    void insertBounded(const QString &text, const QColor &color, const Bound bound);
    void setUpFont(const QFont &font);

protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void updateLineNumberArea();

private:
    LineNumberArea *lineNumberArea;
    void highlightCurrentLine();
    const QTextCharFormat defaultFormat = textCursor().charFormat();
    QTextBlock highlightedBlock;
};

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(DiffTextEdit *editor) : QWidget(editor), editor(editor) {}

    QSize sizeHint() const override { return QSize(editor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override { editor->lineNumberAreaPaintEvent(event); }

private:
    DiffTextEdit *editor;
};

#endif // LINEDIFFWIDGET_H
