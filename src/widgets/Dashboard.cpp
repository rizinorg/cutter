#include "Dashboard.h"
#include "ui_Dashboard.h"
#include "common/Helpers.h"
#include "common/JsonModel.h"
#include "common/JsonTreeItem.h"
#include "common/TempConfig.h"
#include "dialogs/VersionInfoDialog.h"

#include "core/MainWindow.h"
#include "CutterTreeView.h"

#include <QDebug>
#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QLayoutItem>
#include <QString>
#include <QMessageBox>
#include <QDialog>
#include <QTreeWidget>

Dashboard::Dashboard(MainWindow *main) : CutterDockWidget(main), ui(new Ui::Dashboard)
{
    ui->setupUi(this);

    connect(Core(), &CutterCore::refreshAll, this, &Dashboard::updateContents);
}

Dashboard::~Dashboard() {}

void Dashboard::updateContents()
{
    CutterJson docu = Core()->getFileInfo();
    CutterJson item = docu["core"];
    CutterJson item2 = docu["bin"];

    setPlainText(this->ui->modeEdit, item["mode"].toString());
    setPlainText(this->ui->compilationDateEdit, item2["compiled"].toString());

    if (!item2["relro"].toString().isEmpty()) {
        QString relro = item2["relro"].toString().section(QLatin1Char(' '), 0, 0);
        relro[0] = relro[0].toUpper();
        setPlainText(this->ui->relroEdit, relro);
    } else {
        setPlainText(this->ui->relroEdit, "N/A");
    }

    // Add file hashes, analysis info and libraries
    RzCoreLocked core(Core());

    // Using API directly to get values in dashboard
    RzBinFile *bf = rz_bin_cur(core->bin);
    RzBinInfo *binInfo = rz_bin_get_info(core->bin);

    // Setting text values from the API
    setPlainText(this->ui->fileEdit,binInfo->file);
    setPlainText(this->ui->formatEdit,binInfo->rclass);
    setPlainText(this->ui->typeEdit,binInfo->type);
    setPlainText(this->ui->archEdit,binInfo->arch);
    setPlainText(this->ui->langEdit, binInfo->lang);
    setPlainText(this->ui->classEdit,binInfo->bclass);
    setPlainText(this->ui->machineEdit,binInfo->machine);
    setPlainText(this->ui->osEdit,binInfo->os);
    setPlainText(this->ui->subsysEdit,binInfo->subsystem);
    setPlainText(this->ui->compilerEdit,binInfo->compiler);
    setPlainText(this->ui->bitsEdit,QString::number(binInfo->bits));
    setPlainText(this->ui->baddrEdit, RzAddressString(rz_bin_file_get_baddr(bf)));
    setPlainText(this->ui->sizeEdit, qhelpers::formatBytecount(bf->size));
    setPlainText(this->ui->fdEdit, QString::number(bf->fd));

    // Setting the value of "Endianness"
    const char *endian = binInfo->big_endian ? "BE" : "LE";
    setPlainText(this->ui->endianEdit, endian);

    // Setting boolean values
    setBoolvalues(binInfo);

    // Setting the value of "static"
    int static_value = rz_bin_is_static(core->bin);
    setPlainText(ui->staticEdit, tr(intToText(static_value)));

    RzList *hashes = rz_bin_file_compute_hashes(core->bin, bf, UT64_MAX);

    // Delete hashesWidget if it isn't null to avoid duplicate components
    if (hashesWidget) {
        hashesWidget->deleteLater();
    }

    // Define dynamic components to hold the hashes
    hashesWidget = new QWidget();
    QFormLayout *hashesLayout = new QFormLayout;
    hashesWidget->setLayout(hashesLayout);
    ui->hashesVerticalLayout->addWidget(hashesWidget);

    // Add hashes as a pair of Hash Name : Hash Value.
    RzListIter *iter;
    RzBinFileHash *hash;
    CutterRzListForeach (hashes, iter, RzBinFileHash, hash) {
        // Create a bold QString with the hash name uppercased
        QString label = QString("<b>%1:</b>").arg(QString(hash->type).toUpper());

        // Define a Read-Only line edit to display the hash value
        QLineEdit *hashLineEdit = new QLineEdit();
        hashLineEdit->setReadOnly(true);
        hashLineEdit->setText(hash->hex);

        // Set cursor position to begining to avoid long hashes (e.g sha256)
        // to look truncated at the begining
        hashLineEdit->setCursorPosition(0);

        // Add both controls to a form layout in a single row
        hashesLayout->addRow(new QLabel(label), hashLineEdit);
    }

    CutterJson analinfo = Core()->cmdj("aaij");
    setPlainText(ui->functionsLineEdit, QString::number(analinfo["fcns"].toSt64()));
    setPlainText(ui->xRefsLineEdit, QString::number(analinfo["xrefs"].toSt64()));
    setPlainText(ui->callsLineEdit, QString::number(analinfo["calls"].toSt64()));
    setPlainText(ui->stringsLineEdit, QString::number(analinfo["strings"].toSt64()));
    setPlainText(ui->symbolsLineEdit, QString::number(analinfo["symbols"].toSt64()));
    setPlainText(ui->importsLineEdit, QString::number(analinfo["imports"].toSt64()));
    setPlainText(ui->coverageLineEdit, QString::number(analinfo["covrage"].toSt64()) + " bytes");
    setPlainText(ui->codeSizeLineEdit, QString::number(analinfo["codesz"].toSt64()) + " bytes");
    setPlainText(ui->percentageLineEdit, QString::number(analinfo["percent"].toSt64()) + "%");

    QStringList libs = Core()->cmdList("il");
    if (!libs.isEmpty()) {
        libs.removeFirst();
        libs.removeLast();
    }

    // dunno: why not label->setText(lines.join("\n")?
    while (ui->verticalLayout_2->count() > 0) {
        QLayoutItem *item = ui->verticalLayout_2->takeAt(0);
        if (item != nullptr) {
            QWidget *w = item->widget();
            if (w != nullptr) {
                w->deleteLater();
            }

            delete item;
        }
    }

    for (const QString &lib : libs) {
        QLabel *label = new QLabel(this);
        label->setText(lib);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        ui->verticalLayout_2->addWidget(label);
    }

    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui->verticalLayout_2->addSpacerItem(spacer);

    // Check if signature info and version info available
    if (!Core()->getSignatureInfo().size()) {
        ui->certificateButton->setEnabled(false);
    }
    if (!Core()->getFileVersionInfo().size()) {
        ui->versioninfoButton->setEnabled(false);
    }
}

void Dashboard::on_certificateButton_clicked()
{
    static QDialog *viewDialog = nullptr;
    static CutterTreeView *view = nullptr;
    static JsonModel *model = nullptr;
    static QString qstrCertificates;
    if (!viewDialog) {
        viewDialog = new QDialog(this);
        view = new CutterTreeView(viewDialog);
        model = new JsonModel();
        qstrCertificates = Core()->getSignatureInfo().toJson();
    }
    if (!viewDialog->isVisible()) {
        std::string strCertificates = qstrCertificates.toUtf8().constData();
        model->loadJson(QByteArray::fromStdString(strCertificates));
        view->setModel(model);
        view->expandAll();
        view->resize(900, 600);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(view->sizePolicy().hasHeightForWidth());
        viewDialog->setSizePolicy(sizePolicy);
        viewDialog->setMinimumSize(QSize(900, 600));
        viewDialog->setMaximumSize(QSize(900, 600));
        viewDialog->setSizeGripEnabled(false);
        viewDialog->setWindowTitle("Certificates");
        viewDialog->show();
    }
}

void Dashboard::on_versioninfoButton_clicked()
{

    static QDialog *infoDialog = nullptr;

    if (!infoDialog) {
        infoDialog = new VersionInfoDialog(this);
    }

    if (!infoDialog->isVisible()) {
        infoDialog->show();
    }
}

/**
 * @brief Set the text of a QLineEdit. If no text, then "N/A" is set.
 * @param textBox
 * @param text
 */
void Dashboard::setPlainText(QLineEdit *textBox, const QString &text)
{
    if (!text.isEmpty()) {
        textBox->setText(text);
    } else {
        textBox->setText(tr("N/A"));
    }

    textBox->setCursorPosition(0);
}

/**
 * @brief Set the text of a QLineEdit as True, False or N/A if it does not exist
 * @param textBox
 * @param isTrue
 */
void Dashboard::setBool(QLineEdit *textBox, const CutterJson &jsonObject, const char *key)
{
    if (jsonObject[key].valid()) {
        if (jsonObject[key].toBool()) {
            setPlainText(textBox, tr("True"));
        } else {
            setPlainText(textBox, tr("False"));
        }
    } else {
        setPlainText(textBox, tr("N/A"));
    }
}

/**
 * @brief Setting boolean values text according to parsed value from API
 * @param RzBinInfo
 */

void Dashboard::setBoolvalues(RzBinInfo *binInfo)
{
    int nx = binInfo->has_nx;
    int pic = binInfo->has_pi;
    int va = binInfo->has_va;
    int canary = binInfo->has_canary;
    int crypto = binInfo->has_crypto;

    // Setting text
    setPlainText(ui->vaEdit, tr(intToText(va)));
    setPlainText(ui->canaryEdit, tr(intToText(canary)));
    setPlainText(ui->cryptoEdit, tr(intToText(crypto)));
    setPlainText(ui->nxEdit, tr(intToText(nx)));
    setPlainText(ui->picEdit, tr(intToText(pic)));

    // Setting the value of "relocs" and "stripped"
    bool relocs_value = RZ_BIN_DBG_RELOCS & binInfo->dbg_info;
    bool stripped_value = RZ_BIN_DBG_STRIPPED & binInfo->dbg_info;
    setPlainText(this->ui->strippedEdit,QVariant(stripped_value).toString());
    setPlainText(this->ui->relocsEdit,QVariant(relocs_value).toString());
}

/**
 * @brief Set the text of a QLineEdit as True, False according to the integer value
 * @param integer value
 */

char* Dashboard::intToText(int value){
    if (value){
        return "True";
    }
    else if (!value){
        return "False";
    }
}
