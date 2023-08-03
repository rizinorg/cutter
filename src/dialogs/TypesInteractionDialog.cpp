#include "dialogs/TypesInteractionDialog.h"
#include "ui_TypesInteractionDialog.h"

#include "core/Cutter.h"
#include "common/Configuration.h"
#include "common/SyntaxHighlighter.h"
#include "widgets/TypesWidget.h"

#include <QFileDialog>
#include <QTemporaryFile>

TypesInteractionDialog::TypesInteractionDialog(QWidget *parent, bool readOnly)
    : QDialog(parent), ui(new Ui::TypesInteractionDialog)
{
    ui->setupUi(this);
    QFont font = Config()->getBaseFont();
    ui->plainTextEdit->setFont(font);
    ui->plainTextEdit->setPlainText("");
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    ui->plainTextEdit->setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));
#endif
    syntaxHighLighter = Config()->createSyntaxHighlighter(ui->plainTextEdit->document());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->plainTextEdit->setReadOnly(readOnly);
}

TypesInteractionDialog::~TypesInteractionDialog() {}

void TypesInteractionDialog::setTypeName(QString name)
{
    this->typeName = name;
}

void TypesInteractionDialog::on_selectFileButton_clicked()
{
    QString filename =
            QFileDialog::getOpenFileName(this, tr("Select file"), Config()->getRecentFolder(),
                                         "Header files (*.h *.hpp);;All files (*)");
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
        RzCoreLocked core(Core());
        bool success;
        if (!typeName.isEmpty()) {
            success = rz_type_db_edit_base_type(
                    core->analysis->typedb, this->typeName.toUtf8().constData(),
                    ui->plainTextEdit->toPlainText().toUtf8().constData());
        } else {
            char *error_msg = NULL;
            success = rz_type_parse_string_stateless(
                              core->analysis->typedb->parser,
                              ui->plainTextEdit->toPlainText().toUtf8().constData(), &error_msg)
                    == 0;
            if (error_msg) {
                RZ_LOG_ERROR("%s\n", error_msg);
                rz_mem_free(error_msg);
            }
        }
        if (success) {
            emit newTypesLoaded();
            QDialog::done(r);
            return;
        }

        QMessageBox popup(this);
        popup.setWindowTitle(tr("Error"));
        popup.setText(tr("There was some error while loading new types"));
        popup.setStandardButtons(QMessageBox::Ok);
        popup.exec();
    } else {
        QDialog::done(r);
    }
}

void TypesInteractionDialog::fillTextArea(QString content)
{
    ui->layoutWidget->hide();
    ui->plainTextEdit->setPlainText(content);
}
