#ifndef MARKDIALOG_H
#define MARKDIALOG_H

#include "CutterCommon.h"
#include <QDialog>
#include <QColor>

constexpr qreal MARK_ALPHA_F = 0.5; // 50% alpha to show blending of multiple overalapping marks

namespace Ui {
class MarkDialog;
}

class MarkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MarkDialog(RVA from, RVA to, QWidget *parent = nullptr, QString name = {});
    ~MarkDialog();

    void accept() override;

private slots:
    void onPickColor();

private:
    std::unique_ptr<Ui::MarkDialog> ui;
    QString markName;
    RVA markFrom;
    RVA markTo;
    QColor markColor;
    QString markComment;
    bool edit;

    static QString colorToStyle(const QColor &color);
};

#endif // MARKDIALOG_H
