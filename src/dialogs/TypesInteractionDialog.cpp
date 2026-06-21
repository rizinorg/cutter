#include "dialogs/TypesInteractionDialog.h"

#include "common/Configuration.h"
#include "core/Cutter.h"
#include "ui_TypesInteractionDialog.h"

#include <QFileDialog>
#include <QTemporaryFile>

#include <utility>

TypesInteractionDialog::TypesInteractionDialog(QWidget *parent, bool readOnly)
    : QDialog(parent), ui(new Ui::TypesInteractionDialog)
{
    ui->setupUi(this);
    const QFont font = Config()->getBaseFont();
    ui->plainTextEdit->setFont(font);
    ui->plainTextEdit->setPlainText("");
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    ui->plainTextEdit->setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));
#endif

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->plainTextEdit->setReadOnly(readOnly);

    syntaxHighLighter = Config()->createSyntaxHighlighter(ui->plainTextEdit->document());

    connect(ui->selectFileButton, &QPushButton::clicked, this,
            &TypesInteractionDialog::onSelectFileButtonClicked);
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this,
            &TypesInteractionDialog::onPlainTextEditTextChanged);
}

TypesInteractionDialog::~TypesInteractionDialog() {}

void TypesInteractionDialog::setTypeName(QString name)
{
    this->typeName = std::move(name);
}

void TypesInteractionDialog::onSelectFileButtonClicked()
{
    const QString filename =
            QFileDialog::getOpenFileName(this, tr("Select file"), Config()->getRecentFolder(),
                                         "Header files (*.h *.hpp);;All files (*)");
    if (filename.isEmpty()) {
        return;
    }
    Config()->setRecentFolder(QFileInfo(filename).absolutePath());
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        onSelectFileButtonClicked();
        return;
    }
    ui->filenameLineEdit->setText(filename);
    ui->plainTextEdit->setPlainText(file.readAll());
}

void TypesInteractionDialog::onPlainTextEditTextChanged()
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
        RzTypeDB *typedb = rz_analysis_get_type_db(core->analysis);
        if (!typeName.isEmpty()) {
            success = rz_type_db_edit_base_type(
                    typedb, this->typeName.toUtf8().constData(),
                    ui->plainTextEdit->toPlainText().toUtf8().constData());
        } else {
            char *errorMsg = nullptr;
            success = rz_type_parse_string_stateless(
                              typedb->parser, ui->plainTextEdit->toPlainText().toUtf8().constData(),
                              &errorMsg)
                    == 0;
            if (errorMsg) {
                RZ_LOG_ERROR("%s\n", errorMsg);
                rz_mem_free(errorMsg);
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

void TypesInteractionDialog::fillTextArea(const QString &content)
{
    ui->layoutWidget->hide();
    ui->plainTextEdit->setPlainText(content);
}
