#include "dialogs/TypesInteractionDialog.h"
#include "ui_TypesInteractionDialog.h"

#include "core/Cutter.h"
#include "common/Configuration.h"
#include "common/SyntaxHighlighter.h"
#include "widgets/TypesWidget.h"

#include <QFileDialog>
#include <QTemporaryFile>

TypesInteractionDialog::TypesInteractionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TypesInteractionDialog)
{
    ui->setupUi(this);
    ui->plainTextEdit->setPlainText("");
    syntaxHighLighter = new SyntaxHighlighter(ui->plainTextEdit->document());

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

TypesInteractionDialog::~TypesInteractionDialog() {}

void TypesInteractionDialog::on_selectFileButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select file"), Config()->getRecentFolder(), "Header files (*.h *.hpp);;All files (*)");
    if (filename.isEmpty()) {
        return;
    }
    Config()->setRecentFolder(QFileInfo(filename).absolutePath());
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        on_selectFileButton_clicked();
        return;
    }
    ui->filenameLineEdit->setText(filename);
    ui->plainTextEdit->setPlainText(file.readAll());
}

void TypesInteractionDialog::on_plainTextEdit_textChanged()
{
    if (ui->plainTextEdit->toPlainText().isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void TypesInteractionDialog::done(int r)
{
    if (r == QDialog::Accepted) {
        QString error = Core()->addTypes(ui->plainTextEdit->toPlainText());
        if (error.isEmpty()) {
            emit newTypesLoaded();
            QDialog::done(r);
            return;
        }

        QMessageBox popup(this);
        popup.setWindowTitle(tr("Error"));
        popup.setText(tr("There was some error while loading new types"));
        popup.setDetailedText(error);
        popup.setStandardButtons(QMessageBox::Ok);
        popup.exec();
    } else {
        QDialog::done(r);
    }
}

void TypesInteractionDialog::fillTextArea(QString content, bool readonly) {
    ui->layoutWidget->hide();
    ui->plainTextEdit->setPlainText(content);
    if (readonly) {
        ui->plainTextEdit->setReadOnly(true);
    }
}