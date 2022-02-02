#include "YaraAddMetaDialog.h"

#include <QToolTip>
#include <QStringList>
#include <QIntValidator>
#include <QRegExpValidator>
#include "core/Cutter.h"

QStringList YaraAddMetaDialog::FileKeywords = {
    "crc32", "entropy", "md5", "sha1", "sha2", "sha256"
};
QStringList YaraAddMetaDialog::DateKeywords = { "date", "time", "timestamp", "creation" };

bool YaraAddMetaDialog::isKeyword(const QString &keyword)
{
    return YaraAddMetaDialog::FileKeywords.contains(keyword, Qt::CaseInsensitive)
            || YaraAddMetaDialog::DateKeywords.contains(keyword, Qt::CaseInsensitive);
}

YaraAddMetaDialog::YaraAddMetaDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::YaraAddMetaDialog)
{
    // Setup UI
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    auto nameValidator = new QRegExpValidator(QRegExp("^[A-Za-z0-9_]+$"), this);
    ui->nameEdit->setValidator(nameValidator);
    ui->nameEdit->setMaxLength(128);
    ui->valueEdit->setMaxLength(128);

    ui->labelAction->setText(tr("Keywords with auto fill property:\nFile: %1\nDate: %2")
                                     .arg(FileKeywords.join(", "))
                                     .arg(DateKeywords.join(", ")));

    // Connect slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &YaraAddMetaDialog::buttonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
            &YaraAddMetaDialog::buttonBoxRejected);
    connect(ui->nameEdit, &QLineEdit::textChanged, this, &YaraAddMetaDialog::nameChanged);
}

YaraAddMetaDialog::~YaraAddMetaDialog() {}

void YaraAddMetaDialog::nameChanged(const QString &name)
{
    if (YaraAddMetaDialog::isKeyword(name)) {
        ui->valueEdit->setReadOnly(true);
        ui->valueEdit->setText(tr("Auto fill enabled."));
        QToolTip::showText(ui->nameEdit->mapToGlobal(QPoint()),
                           tr("Keyword with auto fill property detected!"));
    } else {
        ui->valueEdit->setReadOnly(false);
        QToolTip::hideText();
    }
}

void YaraAddMetaDialog::buttonBoxAccepted()
{
    QString name = ui->nameEdit->text();
    QString value = YaraAddMetaDialog::isKeyword(name) ? "" : ui->valueEdit->text();

    if (!name.isEmpty()) {
        Core()->cmd("yarama " + name + " '" + value + "'");
    }
    close();
    this->setResult(QDialog::Accepted);
}

void YaraAddMetaDialog::buttonBoxRejected()
{
    close();
    this->setResult(QDialog::Rejected);
}
