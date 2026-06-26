#include "AboutDialog.h"

#include "CutterConfig.h"
#include "RizinPluginsDialog.h"
#include "common/BugReporting.h"
#include "common/Configuration.h"
#include "core/Cutter.h"
#include "ui_AboutDialog.h"

#include <QEventLoop>
#include <QJsonObject>
#include <QProgressBar>
#include <QProgressDialog>
#include <QTimer>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <UpdateWorker.h>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    const QString aboutString(
            tr("Version") + " " CUTTER_VERSION_FULL "<br/>" + tr("Using rizin ")
            + Core()->getRizinVersionReadable() + "<br/>" + buildQtVersionString() + "<p><b>"
            + tr("Optional Features:") + "</b><br/>"
            + tr("Python: %1")
                      .arg(
#ifdef CUTTER_ENABLE_PYTHON
                              tr("ON")
#else
                              tr("OFF")
#endif
                                      )
            + "<br/>"
            + tr("Python Bindings: %1")
                      .arg(
#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
                              tr("ON")
#else
                              tr("OFF")
#endif
                                      )
            + "</p>" + "<h2>" + tr("License") + "</h2>"
            + tr("This Software is released under the GNU General Public License v3.0") + "<h2>"
            + tr("Authors") + "</h2>"
            + tr("Cutter is developed by the community and maintained by its core and development "
                 "teams.<br/>")
            + tr("Check our <a "
                 "href='https://github.com/rizinorg/cutter/graphs/contributors'>contributors "
                 "page</a> for the full list of contributors."));
    ui->label->setText(aboutString);

    const QSignalBlocker s(ui->updatesCheckBox);
    ui->updatesCheckBox->setChecked(Config()->getAutoUpdateEnabled());

    if (!CUTTER_UPDATE_WORKER_AVAILABLE) {
        ui->updatesCheckBox->hide();
        ui->checkForUpdatesButton->hide();
    }

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AboutDialog::onButtonBoxRejected);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    connect(ui->showVersionButton, &QPushButton::clicked, this,
            &AboutDialog::onShowVersionButtonClicked);
    connect(ui->showPluginsButton, &QPushButton::clicked, this,
            &AboutDialog::onShowPluginsButtonClicked);
    connect(ui->issueButton, &QPushButton::clicked, this, &AboutDialog::onIssueClicked);

    connect(ui->checkForUpdatesButton, &QPushButton::clicked, this,
            &AboutDialog::onCheckForUpdatesButtonClicked);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    connect(ui->updatesCheckBox, &QCheckBox::checkStateChanged, this,
            &AboutDialog::onUpdatesCheckBoxStateChanged);
#else
    connect(ui->updatesCheckBox, &QCheckBox::stateChanged, this,
            &AboutDialog::onUpdatesCheckBoxStateChanged);
#endif
}

AboutDialog::~AboutDialog() {}

void AboutDialog::onButtonBoxRejected()
{
    close();
}

void AboutDialog::onShowVersionButtonClicked()
{
    QMessageBox popup(this);
    popup.setWindowTitle(tr("Rizin version information"));
    popup.setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto versionInformation = Core()->getVersionInformation();
    popup.setText(versionInformation);
    popup.exec();
}

void AboutDialog::onShowPluginsButtonClicked()
{
    RizinPluginsDialog dialog(this);
    dialog.exec();
}
void AboutDialog::onIssueClicked()
{
    openIssue();
}

void AboutDialog::onCheckForUpdatesButtonClicked()
{
#if CUTTER_UPDATE_WORKER_AVAILABLE
    UpdateWorker updateWorker;

    auto parentWindow = this;

    QProgressDialog waitDialog(parentWindow);
    auto *bar = new QProgressBar(&waitDialog);
    bar->setMaximum(0);

    waitDialog.setBar(bar);
    waitDialog.setLabel(new QLabel(tr("Checking for updates..."), &waitDialog));

    connect(&updateWorker, &UpdateWorker::checkComplete, &waitDialog, &QProgressDialog::cancel);
    connect(&updateWorker, &UpdateWorker::checkComplete,
            [&updateWorker, parentWindow](const QVersionNumber &version, const QString &error) {
                if (!error.isEmpty()) {
                    QMessageBox::critical(parentWindow, tr("Error!"), error);
                } else {
                    if (version <= UpdateWorker::currentVersionNumber()) {
                        QMessageBox::information(parentWindow, tr("Version control"),
                                                 tr("Cutter is up to date!"));
                    } else {
                        updateWorker.showUpdateDialog(false);
                    }
                }
            });

    updateWorker.checkCurrentVersion(7000);
    waitDialog.exec();
#endif
}

void AboutDialog::onUpdatesCheckBoxStateChanged(int)
{
    Config()->setAutoUpdateEnabled(!Config()->getAutoUpdateEnabled());
}

static QString compilerString()
{
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
    QString isAppleString; // NOLINT
#    if defined(__apple_build_version__) // Apple clang has other version numbers
    isAppleString = QLatin1String(" (Apple)");
#    endif
    return QLatin1String("Clang ") + QString::number(__clang_major__) + QLatin1Char('.')
            + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC ") + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    if (_MSC_VER > 1999) {
        return QLatin1String("MSVC <unknown>");
    }
    if (_MSC_VER >= 1910) {
        return QLatin1String("MSVC 2017");
    }
    if (_MSC_VER >= 1900) {
        return QLatin1String("MSVC 2015");
    }
#endif
    return QLatin1String("<unknown compiler>");
}

QString AboutDialog::buildQtVersionString(void)
{
    return tr("Based on Qt %1 (%2, %3 bit)")
            .arg(QLatin1String(qVersion()), compilerString(), QString::number(QSysInfo::WordSize));
}
