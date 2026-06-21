#include "RegisterProfileDialog.h"

#include "Configuration.h"
#include "Cutter.h"
#include "EditRegProfileDialog.h"
#include "ui_RegisterProfileDialog.h"

#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>

RegisterProfileDialog::RegisterProfileDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::RegisterProfileDialog)
{
    ui->setupUi(this);

    ui->recentProfilesList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->loadProfileBtn, &QPushButton::clicked, this,
            &RegisterProfileDialog::loadProfileBtnClicked);
    connect(ui->editProfileBtn, &QPushButton::clicked, this,
            &RegisterProfileDialog::editProfileBtnClicked);
    connect(ui->recentProfilesList, &QListWidget::itemClicked, this,
            &RegisterProfileDialog::itemClicked);
    connect(ui->recentProfilesList, &QListWidget::customContextMenuRequested, this,
            &RegisterProfileDialog::showContextMenu);
    connect(ui->loadGDBBtn, &QPushButton::clicked, this, &RegisterProfileDialog::loadGDBBtnClicked);
    connect(ui->exportProfileBtn, &QPushButton::clicked, this,
            &RegisterProfileDialog::exportProfileBtnClicked);
}

RegisterProfileDialog::~RegisterProfileDialog() = default;

void RegisterProfileDialog::showContextMenu(const QPoint &pos)
{
    const QListWidgetItem *item = ui->recentProfilesList->itemAt(pos);
    QMenu menu(this);
    if (item) {
        menu.addAction(tr("Remove Item"), this, &RegisterProfileDialog::removeItem);
        menu.addSeparator();
    }
    menu.addAction(tr("Clear All"), this, &RegisterProfileDialog::clearAll);
    menu.exec(ui->recentProfilesList->mapToGlobal(pos));
}

void RegisterProfileDialog::removeItem()
{
    const QListWidgetItem *item = ui->recentProfilesList->currentItem();
    if (!item) {
        return;
    }

    const QString serializedData = item->data(Qt::UserRole).toString();
    Config()->removeRecentRegProfile(serializedData);
    delete ui->recentProfilesList->takeItem(ui->recentProfilesList->currentRow());
}

void RegisterProfileDialog::clearAll()
{
    ui->recentProfilesList->clear();
    Config()->setRecentRegProfiles(QStringList());
}

void RegisterProfileDialog::fillProfilePaths(const QStringList &profilePaths)
{
    ui->recentProfilesList->clear();

    for (const QString &p : profilePaths) {
        const bool isGdb = p.startsWith("gdb::");
        const QString pathData = p.mid(p.indexOf("::") + 2);

        // Visible format:
        // [Rizin/GDB] /path/to/file
        // easier for user to remember if the opened file was a GDB profile or Rizin
        const QString displayText = QString("[%1] %2").arg(isGdb ? "GDB" : "Rizin", pathData);

        auto *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, p);
        ui->recentProfilesList->addItem(item);
    }
}

void RegisterProfileDialog::loadProfileBtnClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open Register Profile"),
                                                          QString(), tr("All Files (*)"));
    if (!filePath.isEmpty()) {
        setFileContents(filePath);
        loadedProfile = RegisterProfile::Rizin;
    }
}

void RegisterProfileDialog::loadGDBBtnClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open GDB Profile"), QString(),
                                                          tr("All Files (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    const QString profile = Core()->convertGdbProfile(filePath);
    if (profile.isEmpty()) {
        return;
    }

    setProfileData(profile);
    setProfilePath(filePath);
    loadedProfile = RegisterProfile::GDB;
}

void RegisterProfileDialog::setFileContents(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        const QString data = in.readAll();

        setProfileData(data);
        setProfilePath(filePath);
        file.close();
    } else {
        showWarning(filePath);
    }
}

void RegisterProfileDialog::showWarning(const QString &filePath)
{
    QMessageBox::warning(this, tr("Error"), tr("Could not open file: %1").arg(filePath));
}

void RegisterProfileDialog::editProfileBtnClicked()
{
    EditRegProfileDialog dialog(this);
    dialog.setProfileData(profileData);
    if (dialog.exec() == QDialog::Accepted) {
        setProfileData(dialog.getProfleData());
    }
}

void RegisterProfileDialog::setProfileData(const QString &data)
{
    profileData = data;
}

void RegisterProfileDialog::updateProfile(const QString &path, const QString &data)
{
    setProfilePath(path);
    setProfileData(data);
}

QString RegisterProfileDialog::getProfileData() const
{
    return profileData;
}

RegisterProfile RegisterProfileDialog::getLoadedProfile() const
{
    return loadedProfile;
}

QString RegisterProfileDialog::getSerializedProfilePath() const
{
    // Saving format:
    // rizin::/path/to/profile
    // gdb::/path/to/profile
    const QString path = ui->profilePathEdit->text();
    if (path.isEmpty()) {
        return QString();
    }
    return (loadedProfile == RegisterProfile::GDB ? "gdb::" : "rizin::") + path;
}

void RegisterProfileDialog::itemClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    const QString fullSerialized = item->data(Qt::UserRole).toString();
    const QString path = fullSerialized.mid(fullSerialized.indexOf("::") + 2);

    loadedProfile =
            fullSerialized.startsWith("gdb::") ? RegisterProfile::GDB : RegisterProfile::Rizin;

    setFileContents(path);
}

void RegisterProfileDialog::exportProfileBtnClicked()
{
    if (profileData.isEmpty()) {
        QMessageBox::information(this, tr("Export Profile"), tr("The profile is empty."));
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, tr("Export Profile As"), QString(),
                                                          tr("All Files (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << profileData;
        file.close();

        setProfilePath(filePath);
        loadedProfile = RegisterProfile::Rizin;
    } else {
        showWarning(filePath);
    }
}

void RegisterProfileDialog::setProfilePath(const QString &path)
{
    ui->profilePathEdit->setText(path);
}

QString RegisterProfileDialog::getProfilePath() const
{
    return ui->profilePathEdit->text();
}
