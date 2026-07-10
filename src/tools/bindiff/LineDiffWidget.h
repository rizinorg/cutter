#ifndef LINEDIFFWIDGET_H
#define LINEDIFFWIDGET_H

#include <QPlainTextEdit>
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
    void fetchFunctionDisas(RVA addrA, RVA addrB);

protected:
private:
    CutterDiff *cutterDiff;
    DiffTextEdit *leftEdit;
    DiffTextEdit *rightEdit;
};

class DiffTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DiffTextEdit(QWidget *parent = nullptr);
    ~DiffTextEdit();
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void insertFormatted(const QString &text, const QColor &color);

protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void updateLineNumberArea();

private:
    LineNumberArea *lineNumberArea;
    void highlightCurrentLine();
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
