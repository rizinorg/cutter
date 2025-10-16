#include "MarkDialog.h"
#include "Cutter.h"
#include "CutterCommon.h"
#include "ui_MarkDialog.h"
#include <QColorDialog>
#include <QRegularExpressionValidator>

MarkDialog::MarkDialog(RVA start, RVA end, QWidget *parent, QString name)
    : QDialog(parent),
      ui(new Ui::MarkDialog),
      markName(name),
      markFrom(start),
      markTo(end),
      markColor(Qt::black),
      markComment(""),
      edit(false)
{
    ui->setupUi(this);

    if (!markName.isEmpty()) {
        // Editing existing Mark
        setWindowTitle("Edit Mark");
        RzCoreLocked core(Core());
        RzMarkItem *mark = rz_mark_get(core->marks, markName.toStdString().c_str());
        if (mark) {
            markFrom = mark->from;
            markTo = mark->to;
            markColor = QColor(mark->color);
            markComment = mark->comment;
        }
        edit = true;
    } else {
        // Creating new Mark
        setWindowTitle("Add Mark");
        markName = QString("%1_%2").arg(RzAddressString(markFrom), RzAddressString(markTo));
    }

    ui->startAddressEdit->setText(RzAddressString(markFrom));
    ui->endAddressEdit->setText(RzAddressString(markTo));
    ui->nameEdit->setText(markName);
    ui->commentEdit->setText(markComment);
    ui->colorDisplay->setStyleSheet(colorToStyle(markColor));

    auto hexValidator =
            new QRegularExpressionValidator(QRegularExpression("(?:0[xX])?[0-9a-fA-F]+"), this);
    ui->startAddressEdit->setValidator(hexValidator);
    ui->endAddressEdit->setValidator(hexValidator);

    connect(ui->colorButton, &QPushButton::clicked, this, &MarkDialog::onPickColor);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &MarkDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &MarkDialog::reject);
}

void MarkDialog::accept()
{
    bool ok = false;
    markFrom = ui->startAddressEdit->text().toULongLong(&ok, 16);
    if (!ok) {
        QMessageBox::warning(this, tr("Invalid Input"),
                             tr("Starting address is not a valid hexadecimal number"));
        return;
    }
    markTo = ui->endAddressEdit->text().toULongLong(&ok, 16);
    if (!ok) {
        QMessageBox::warning(this, tr("Invalid Input"),
                             tr("Ending address is not a valid hexadecimal number"));
        return;
    }
    if (markFrom > markTo) {
        QMessageBox::warning(this, tr("Invalid Input"),
                             tr("Starting address cannot be greater than ending address"));
        return;
    }

    QString name = ui->nameEdit->text();
    if (edit && !name.isEmpty() && name != markName) {
        Core()->delMark(markName); // Delete the old mark
    } else if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Name cannot be empty"));
        return;
    }

    markName = name;
    markComment = ui->commentEdit->toPlainText();
    Core()->addMark(markFrom, markTo, markName, markComment, markColor);

    QDialog::accept();
}

MarkDialog::~MarkDialog() {}

void MarkDialog::onPickColor()
{
    QColor c = QColorDialog::getColor(markColor, this, tr("Pick Background Color"));
    if (c.isValid()) {
        markColor = c;
        ui->colorDisplay->setStyleSheet(colorToStyle(markColor));
    }
}

QString MarkDialog::colorToStyle(const QColor &color)
{
    return QString("background-color: rgba(%1, %2, %3, %4);")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(MARK_ALPHA_F);
}
