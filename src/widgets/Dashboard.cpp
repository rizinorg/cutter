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
    RzCoreLocked core(Core());
    int fd = rz_io_fd_get_current(core->io);
    RzIODesc *desc = rz_io_desc_get(core->io, fd);
    RzBinFile *bf = rz_bin_cur(core->bin);

    setPlainText(this->ui->modeEdit, desc ? rz_str_rwx_i(desc->perm & RZ_PERM_RWX) : "");
    setPlainText(this->ui->compilationDateEdit, rz_core_bin_get_compile_time(bf));

    char *relco_buf = sdb_get(bf->o->kv, "elf.relro", 0);
    if (RZ_STR_ISNOTEMPTY(relco_buf)) {
        QString relro = QString(relco_buf).section(QLatin1Char(' '), 0, 0);
        relro[0] = relro[0].toUpper();
        setPlainText(this->ui->relroEdit, relro);
    } else {
        setPlainText(this->ui->relroEdit, "N/A");
    }

    // Add file hashes, analysis info and libraries
    RzBinInfo *binInfo = rz_bin_get_info(core->bin);

    setPlainText(ui->fileEdit, binInfo ? binInfo->file : "");
    setPlainText(ui->formatEdit, binInfo ? binInfo->rclass : "");
    setPlainText(ui->typeEdit, binInfo ? binInfo->type : "");
    setPlainText(ui->archEdit, binInfo ? binInfo->arch : "");
    setPlainText(ui->langEdit, binInfo ? binInfo->lang : "");
    setPlainText(ui->classEdit, binInfo ? binInfo->bclass : "");
    setPlainText(ui->machineEdit, binInfo ? binInfo->machine : "");
    setPlainText(ui->osEdit, binInfo ? binInfo->os : "");
    setPlainText(ui->subsysEdit, binInfo ? binInfo->subsystem : "");
    setPlainText(ui->compilerEdit, binInfo ? binInfo->compiler : "");
    setPlainText(ui->bitsEdit, binInfo ? QString::number(binInfo->bits) : "");
    setPlainText(ui->baddrEdit, binInfo ? RzAddressString(rz_bin_file_get_baddr(bf)) : "");
    setPlainText(ui->sizeEdit, binInfo ? qhelpers::formatBytecount(bf->size) : "");
    setPlainText(ui->fdEdit, binInfo ? QString::number(bf->fd) : "");

    // Setting the value of "Endianness"
    const char *endian = binInfo ? (binInfo->big_endian ? "BE" : "LE") : "";
    setPlainText(this->ui->endianEdit, endian);

    // Setting boolean values
    setRzBinInfo(binInfo);

    // Setting the value of "static"
    int static_value = rz_bin_is_static(core->bin);
    setPlainText(ui->staticEdit, tr(setBoolText(static_value)));

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

    st64 fcns = rz_list_length(core->analysis->fcns);
    st64 strs = rz_flag_count(core->flags, "str.*");
    st64 syms = rz_flag_count(core->flags, "sym.*");
    st64 imps = rz_flag_count(core->flags, "sym.imp.*");
    st64 code = rz_core_analysis_code_count(core);
    st64 covr = rz_core_analysis_coverage_count(core);
    st64 call = rz_core_analysis_calls_count(core);
    ut64 xrfs = rz_analysis_xrefs_count(core->analysis);
    double precentage = (code > 0) ? (covr * 100.0 / code) : 0;

    setPlainText(ui->functionsLineEdit, QString::number(fcns));
    setPlainText(ui->xRefsLineEdit, QString::number(xrfs));
    setPlainText(ui->callsLineEdit, QString::number(call));
    setPlainText(ui->stringsLineEdit, QString::number(strs));
    setPlainText(ui->symbolsLineEdit, QString::number(syms));
    setPlainText(ui->importsLineEdit, QString::number(imps));
    setPlainText(ui->coverageLineEdit, QString::number(covr) + " bytes");
    setPlainText(ui->codeSizeLineEdit, QString::number(code) + " bytes");
    setPlainText(ui->percentageLineEdit, QString::number(precentage) + "%");

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

    const RzList *libs = rz_bin_object_get_libs(bf->o);
    if (libs) {
        for (const auto &lib : CutterRzList<char>(libs)) {
            auto *label = new QLabel(this);
            label->setText(lib);
            label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            ui->verticalLayout_2->addWidget(label);
        }
    }

    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui->verticalLayout_2->addSpacerItem(spacer);

    // Check if signature info and version info available
    if (Core()->getSignatureInfo().isEmpty()) {
        ui->certificateButton->setEnabled(false);
    }
    const RzBinInfo *info = rz_bin_get_info(core->bin);
    if (!info) {
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
        qstrCertificates = Core()->getSignatureInfo();
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
 * @brief Setting boolean values of binary information in dashboard
 * @param RzBinInfo
 */
void Dashboard::setRzBinInfo(RzBinInfo *binInfo)
{
    setPlainText(ui->vaEdit, binInfo ? setBoolText(binInfo->has_va) : "");
    setPlainText(ui->canaryEdit, binInfo ? setBoolText(binInfo->has_canary) : "");
    setPlainText(ui->cryptoEdit, binInfo ? setBoolText(binInfo->has_crypto) : "");
    setPlainText(ui->nxEdit, binInfo ? setBoolText(binInfo->has_nx) : "");
    setPlainText(ui->picEdit, binInfo ? setBoolText(binInfo->has_pi) : "");
    setPlainText(ui->strippedEdit,
                 binInfo ? setBoolText(RZ_BIN_DBG_STRIPPED & binInfo->dbg_info) : "");
    setPlainText(ui->relocsEdit, binInfo ? setBoolText(RZ_BIN_DBG_RELOCS & binInfo->dbg_info) : "");
}

/**
 * @brief Set the text of a QLineEdit as True, False
 * @param boolean value
 */
const char *Dashboard::setBoolText(bool value)
{
    return value ? "True" : "False";
}
