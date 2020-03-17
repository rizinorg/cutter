#include "RemoteDebugDialog.h"
#include "ui_RemoteDebugDialog.h"

#include <QHostAddress>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QSettings>

#define GDBSERVER "GDB"
#define WINDBGPIPE "WinDbg - Pipe"
#define WINDBG_URI_PREFIX "windbg"
#define GDB_URI_PREFIX "gdb"
#define DEFAULT_INDEX (GDBSERVER)

RemoteDebugDialog::RemoteDebugDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RemoteDebugDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setAcceptDrops(true);
    ui->recentsIpListWidget->addAction(ui->actionRemove_item);
    ui->recentsIpListWidget->addAction(ui->actionClear_all);
    // Set a default selection
    ui->debuggerCombo->setCurrentIndex(ui->debuggerCombo->findText(DEFAULT_INDEX));
    onIndexChange();

    fillRecentIpList();
    
    ui->ipEdit->setFocus();

    connect(ui->debuggerCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &RemoteDebugDialog::onIndexChange);
}

RemoteDebugDialog::~RemoteDebugDialog() {}

bool RemoteDebugDialog::validate()
{
    QString debugger = getDebugger();

    if (debugger == GDBSERVER) {
        if (!validateIp()) {
            return false;
        }
        if (!validatePort()) {
            return false;
        }
    } else if (debugger == WINDBGPIPE) {
        if (!validatePath()) {
            return false;
        }
    } else {
        QMessageBox msgBox;
        msgBox.setText(tr("Invalid debugger"));
        msgBox.exec();
        return false;
    }

    return true;
}

bool RemoteDebugDialog::validateIp()
{
    QMessageBox msgBox;

    QString ip = getIp();
    if (QHostAddress(ip).isNull()) {
        msgBox.setText(tr("Invalid IP address"));
        msgBox.exec();
        return false;
    }
    return true;
}

bool RemoteDebugDialog::validatePath()
{
    QMessageBox msgBox;

    QString path = getPath();
    if (!QFileInfo(path).exists()) {
        msgBox.setText(tr("Path does not exist"));
        msgBox.exec();
        return false;
    }
    return true;
}

bool RemoteDebugDialog::validatePort()
{
    QMessageBox msgBox;

    int port = getPort();
    if (port < 1 || port > 65535) {
        msgBox.setText(tr("Invalid port"));
        msgBox.exec();
        return false;
    }
    return true;
}

void RemoteDebugDialog::onIndexChange()
{
    QString debugger = getDebugger();
    if (debugger == GDBSERVER) {
        activateGdb();
    } else if (debugger == WINDBGPIPE) {
        activateWinDbgPipe();
    }
}

void RemoteDebugDialog::activateGdb()
{
    ui->ipEdit->setVisible(true);
    ui->portEdit->setVisible(true);
    ui->ipText->setVisible(true);
    ui->portText->setVisible(true);
    ui->pathEdit->setVisible(false);
    ui->pathText->setVisible(false);
}

void RemoteDebugDialog::activateWinDbgPipe()
{
    ui->ipEdit->setVisible(false);
    ui->portEdit->setVisible(false);
    ui->ipText->setVisible(false);
    ui->portText->setVisible(false);
    ui->pathEdit->setVisible(true);
    ui->pathText->setVisible(true);
}

void RemoteDebugDialog::on_buttonBox_accepted()
{
}

void RemoteDebugDialog::on_buttonBox_rejected()
{
    close();
}

void RemoteDebugDialog::setIp(QString ip)
{
    ui->ipEdit->setText(ip);
    ui->ipEdit->selectAll();
}

void RemoteDebugDialog::setPath(QString path)
{
    ui->pathEdit->setText(path);
    ui->pathEdit->selectAll();
}

void RemoteDebugDialog::setPort(QString port)
{
    ui->portEdit->setText(port);
    ui->portEdit->selectAll();
}

void RemoteDebugDialog::setDebugger(QString debugger)
{
    ui->debuggerCombo->setCurrentIndex(ui->debuggerCombo->findText(debugger));
}

void RemoteDebugDialog::on_actionRemove_item_triggered()
{
    QListWidgetItem *item = ui->recentsIpListWidget->currentItem();

    if (item == nullptr)
        return;

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    QSettings settings;
    QStringList ips = settings.value("recentFileList").toStringList();
    ips.removeAll(sitem);
    settings.setValue("recentFileList", ips);

    ui->recentsIpListWidget->takeItem(ui->recentsIpListWidget->currentRow());

    ui->ipEdit->clear();

}

void RemoteDebugDialog::on_actionClear_all_triggered()
{
    QSettings settings;
    QStringList ips = settings.value("recentIpList").toStringList();
    ips.clear();

    ui->recentsIpListWidget->clear();
    settings.setValue("recentIpList", ips);
    ui->ipEdit->clear();
}

bool RemoteDebugDialog::fillRecentIpList()
{
    QSettings settings;

    QStringList ips = settings.value("recentIpList").toStringList();
    QMutableListIterator<QString> it(ips);
    while (it.hasNext()) {
        const QString ip = it.next();
        // Format the text and add the item to the file list
        const QString text = QString("%1").arg(ip);
        QListWidgetItem *item = new QListWidgetItem(
            text
        );
        item->setData(Qt::UserRole, ip);
        ui->recentsIpListWidget->addItem(item);
    }
    
    settings.setValue("recentIpList", ips);
    return !ips.isEmpty();
}

void RemoteDebugDialog::on_recentsIpListWidget_itemClicked(QListWidgetItem *item)
{
    QVariant data = item->data(Qt::UserRole);
    QString ipport = data.toString();
    if (!ipport.indexOf(GDB_URI_PREFIX)) {
        ui->debuggerCombo->setCurrentIndex(ui->debuggerCombo->findText(GDBSERVER));
        int last_colon = ipport.lastIndexOf(QString(":"));
        QString port_temp = ipport.mid(last_colon+1,ipport.length());
        QString ip_temp = ipport.mid(6,ipport.length()-port_temp.length()-7);
        ui->ipEdit->setText(ip_temp);
        ui->portEdit->setText(port_temp);
    } else if (!ipport.indexOf(WINDBG_URI_PREFIX)) {
        ui->debuggerCombo->setCurrentIndex(ui->debuggerCombo->findText(WINDBGPIPE));
        QString path_temp = ipport.mid(9,ipport.length());
        ui->pathEdit->setText(path_temp);
    }
}

void RemoteDebugDialog::on_recentsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    // loadFile(item->data(Qt::UserRole).toString());
}

QString RemoteDebugDialog::getUri() const
{
    QString debugger = getDebugger();
    if (debugger == WINDBGPIPE) {
        return QString("%1://%2").arg(WINDBG_URI_PREFIX, getPath());
    } else if (debugger == GDBSERVER) {
        return QString("%1://%2:%3").arg(GDB_URI_PREFIX, getIp(), QString::number(getPort()));
    }

    return NULL;
}

QString RemoteDebugDialog::getIp() const
{
    auto temp = ui->ipEdit->text();
    if (temp == "" || temp == "localhost") {
        return "127.0.0.1";
    } else {
        return temp;
    }
}

QString RemoteDebugDialog::getPath() const
{
    return ui->pathEdit->text();
}

int RemoteDebugDialog::getPort() const
{
    return ui->portEdit->text().toInt();
}

QString RemoteDebugDialog::getDebugger() const
{
    return ui->debuggerCombo->currentText();
}
