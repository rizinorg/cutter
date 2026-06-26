#include "RemoteDebugDialog.h"

#include "CutterCommon.h" // IWYU pragma: keep
#include "ui_RemoteDebugDialog.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QMessageBox>
#include <QSettings>

enum DbgBackendType : ut8 { GDB = 0, WINDBG = 1 };

struct DbgBackend
{
    DbgBackendType type;
    QString name;
    QString prefix;
};

static const QList<DbgBackend> dbgBackends = { { GDB, "GDB", "gdb://" },
                                               { WINDBG, "WinKd - Pipe", "winkd://" } };

RemoteDebugDialog::RemoteDebugDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::RemoteDebugDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Fill in debugger Combo
    ui->debuggerCombo->clear();
    for (auto &backend : dbgBackends) {
        ui->debuggerCombo->addItem(backend.name);
    }

    // Fill ip list
    fillRecentIpList();
    ui->ipEdit->setFocus();

    // Connect statements for right click action and itemClicked action
    ui->recentsIpListWidget->addAction(ui->actionRemoveItem);
    ui->recentsIpListWidget->addAction(ui->actionRemoveAll);
    connect(ui->actionRemoveAll, &QAction::triggered, this, &RemoteDebugDialog::clearAll);
    connect(ui->actionRemoveItem, &QAction::triggered, this, &RemoteDebugDialog::removeItem);
    connect(ui->recentsIpListWidget, &QListWidget::itemClicked, this,
            &RemoteDebugDialog::itemClicked);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &RemoteDebugDialog::onButtonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
            &RemoteDebugDialog::onButtonBoxRejected);
}

RemoteDebugDialog::~RemoteDebugDialog() {}

bool RemoteDebugDialog::validate()
{
    const int debugger = getDebugger();
    if (debugger == GDB) {
        return validatePort() && validateIp();
    } else if (debugger == WINDBG) {
        return validatePath();
    }
    QMessageBox msgBox(this);
    msgBox.setText(tr("Invalid debugger"));
    msgBox.exec();
    return false;
}

bool RemoteDebugDialog::validateIp()
{
    QMessageBox msgBox(this);

    const QString ip = getIpOrPath();
    if (QHostAddress(ip).isNull()) {
        msgBox.setText(tr("Invalid IP address"));
        msgBox.exec();
        return false;
    }
    return true;
}

bool RemoteDebugDialog::validatePath()
{
    QMessageBox msgBox(this);

    const QString path = getIpOrPath();
    if (!QFileInfo::exists(path)) {
        msgBox.setText(tr("Path does not exist"));
        msgBox.exec();
        return false;
    }
    return true;
}

bool RemoteDebugDialog::validatePort()
{
    QMessageBox msgBox(this);

    const int port = getPort();
    if (port < 1 || port > 65535) {
        msgBox.setText(tr("Invalid port"));
        msgBox.exec();
        return false;
    }
    return true;
}

void RemoteDebugDialog::onButtonBoxAccepted() {}

void RemoteDebugDialog::onButtonBoxRejected()
{
    close();
}

void RemoteDebugDialog::removeItem()
{
    const QListWidgetItem *item = ui->recentsIpListWidget->currentItem();

    if (item == nullptr) {
        return;
    }

    const QVariant data = item->data(Qt::UserRole);
    const QString sitem = data.toString();

    // Remove the item from recentIpList
    QSettings settings;
    QStringList ips = settings.value("recentIpList").toStringList();
    ips.removeAll(sitem);
    settings.setValue("recentIpList", ips);

    // Also remove the line from list
    ui->recentsIpListWidget->takeItem(ui->recentsIpListWidget->currentRow());
    checkIfEmpty();
}

void RemoteDebugDialog::clearAll()
{
    QSettings settings;
    ui->recentsIpListWidget->clear();

    QStringList ips = settings.value("recentIpList").toStringList();
    ips.clear();
    settings.setValue("recentIpList", ips);

    checkIfEmpty();
}

void RemoteDebugDialog::fillFormData(const QString &formdata)
{
    QString ipText = "";
    QString portText = "";
    const DbgBackend *backend = nullptr;
    for (auto &back : dbgBackends) {
        if (formdata.startsWith(back.prefix)) {
            backend = &back;
        }
    }
    if (!backend) {
        return;
    }

    if (backend->type == GDB) {
        // Format is | prefix | IP | : | PORT |
        const int lastColon = formdata.lastIndexOf(QString(":"));
        portText = formdata.mid(lastColon + 1, formdata.length());
        ipText = formdata.mid(backend->prefix.length(), lastColon - backend->prefix.length());
    } else if (backend->type == WINDBG) {
        // Format is | prefix | PATH |
        ipText = formdata.mid(backend->prefix.length());
    }
    ui->debuggerCombo->setCurrentText(backend->name);
    ui->ipEdit->setText(ipText);
    ui->portEdit->setText(portText);
}

QString RemoteDebugDialog::getUri() const
{
    const int debugger = getDebugger();
    if (debugger == WINDBG) {
        return QString("%1%2").arg(dbgBackends[WINDBG].prefix, getIpOrPath());
    } else if (debugger == GDB) {
        return QString("%1%2:%3").arg(dbgBackends[GDB].prefix, getIpOrPath(),
                                      QString::number(getPort()));
    }
    return "- uri error";
}

bool RemoteDebugDialog::fillRecentIpList()
{
    const QSettings settings;

    // Fetch recentIpList
    QStringList ips = settings.value("recentIpList").toStringList();
    QMutableListIterator<QString> it(ips);
    while (it.hasNext()) {
        const QString ip = it.next();
        const QString text = QString("%1").arg(ip);
        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, ip);
        // Fill recentsIpListWidget
        ui->recentsIpListWidget->addItem(item);
    }

    if (!ips.isEmpty()) {
        fillFormData(ips[0]);
    }

    checkIfEmpty();

    return !ips.isEmpty();
}

void RemoteDebugDialog::checkIfEmpty()
{
    const QSettings settings;
    const QStringList ips = settings.value("recentIpList").toStringList();

    if (ips.isEmpty()) {
        ui->recentsIpListWidget->setVisible(false);
        ui->line->setVisible(false);
    } else {
        // TODO: Find a way to make the list widget not to high
    }
}

void RemoteDebugDialog::itemClicked(QListWidgetItem *item)
{
    const QVariant data = item->data(Qt::UserRole);
    const QString ipport = data.toString();
    fillFormData(ipport);
}

QString RemoteDebugDialog::getIpOrPath() const
{
    return ui->ipEdit->text();
}

int RemoteDebugDialog::getPort() const
{
    return ui->portEdit->text().toInt();
}

int RemoteDebugDialog::getDebugger() const
{
    return ui->debuggerCombo->currentIndex();
}
