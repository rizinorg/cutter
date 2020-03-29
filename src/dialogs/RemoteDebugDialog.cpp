#include "RemoteDebugDialog.h"
#include "ui_RemoteDebugDialog.h"

#include <QHostAddress>
#include <QFileInfo>
#include <QMessageBox>
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
    ui->debuggerCombo->setCurrentText(DEFAULT_INDEX);
    onIndexChange();

    fillRecentIpList();

    ui->ipEdit->setFocus();

    connect(ui->debuggerCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &RemoteDebugDialog::onIndexChange);

    // connect statements for right click action and itemClicked action
    connect(ui->actionClear_all, &QAction::triggered, this, &RemoteDebugDialog::clearAll);
    connect(ui->actionRemove_item, &QAction::triggered, this, &RemoteDebugDialog::removeItem);
    connect(ui->recentsIpListWidget, &QListWidget::itemClicked, this, &RemoteDebugDialog::itemClicked);
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

/**
* @brief Clears the selected item in the list of recent connections.
* Triggers when you right click and click on "Remove Item" in remote debug dialog.
*/
void RemoteDebugDialog::removeItem()
{
    QListWidgetItem *item = ui->recentsIpListWidget->currentItem();

    if (item == nullptr)
        return;

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    // remove the item from recentIpList
    QSettings settings;
    QStringList ips = settings.value("recentIpList").toStringList();
    ips.removeAll(sitem);
    settings.setValue("recentIpList", ips);

    // also remove the line from list
    ui->recentsIpListWidget->takeItem(ui->recentsIpListWidget->currentRow());
    checkIfEmpty();
}

/**
* @brief Clears the list of recent connections.
* Triggers when you right click and click on "Clear All" in remote debug dialog.
*/
void RemoteDebugDialog::clearAll()
{
    QSettings settings;
    // empty QStringList
    QStringList temp;
    ui->recentsIpListWidget->clear();
    settings.setValue("recentIpList", temp);
    checkIfEmpty();
}

/**
* @brief Fills the remote debug dialog form with given string
* Eg: gdb://127.0.0.1:8754 or windbg:///tmp/abcd
*/
void RemoteDebugDialog::fillFormData(QString formdata)
{
    // the PREFIX should be in start
    if (formdata.startsWith(GDB_URI_PREFIX)) {
        ui->debuggerCombo->setCurrentText(GDBSERVER);
        int last_colon = formdata.lastIndexOf(QString(":"));
        QString port_temp = formdata.mid(last_colon + 1, formdata.length());
        // +/- 3 is because of "://" in path/url
        QString ip_temp = formdata.mid(QString(GDB_URI_PREFIX).length() + 3,
                                       formdata.length() - port_temp.length() - QString(GDB_URI_PREFIX).length() - 3 - 1);
        ui->ipEdit->setText(ip_temp);
        ui->portEdit->setText(port_temp);
    } else if (formdata.startsWith(WINDBG_URI_PREFIX)) {
        ui->debuggerCombo->setCurrentText(WINDBGPIPE);
        QString path_temp = formdata.mid(QString(WINDBG_URI_PREFIX).length() + 3, formdata.length());
        ui->pathEdit->setText(path_temp);
    }
}

/**
* @brief Fills recent remote connections.
*/
bool RemoteDebugDialog::fillRecentIpList()
{
    QSettings settings;

    // fetch recentIpList
    QStringList ips = settings.value("recentIpList").toStringList();
    QMutableListIterator<QString> it(ips);
    while (it.hasNext()) {
        const QString ip = it.next();
        const QString text = QString("%1").arg(ip);
        QListWidgetItem *item = new QListWidgetItem(
            text
        );
        item->setData(Qt::UserRole, ip);
        // fill recentsIpListWidget
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
    QSettings settings;
    QStringList ips = settings.value("recentIpList").toStringList();

    if (ips.isEmpty()) {
        ui->recentsIpListWidget->setVisible(false);
        ui->line->setVisible(false);

        auto finalHeight = ui->line->size().height() + ui->recentsIpListWidget->size().height();
        if (finalHeight > 90){
            resize(size().width(), size().height() - finalHeight);
        }
    }
}

/**
* @brief Fills the form with the selected item's data.
*/
void RemoteDebugDialog::itemClicked(QListWidgetItem *item)
{
    QVariant data = item->data(Qt::UserRole);
    QString ipport = data.toString();

    fillFormData(ipport);
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
    // empty text => "127.0.0.1"
    auto temp = ui->ipEdit->text();
    if (temp == "") {
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
