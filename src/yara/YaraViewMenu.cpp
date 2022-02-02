#include "YaraViewMenu.h"
#include <MainWindow.h>

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

YaraViewMenu::YaraViewMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent), mainWindow(mainWindow)
{
    actionAddNewMetadata = new QAction(tr("Add New Entry"), this);
    actionCopyName = new QAction(tr("Copy Name"), this);
    actionSeekAt = new QAction(tr("Seek At"), this);
    actionRemove = new QAction(tr("Remove Entry"), this);
    actionRemoveAll = new QAction(tr("Remove All Entries"), this);

    connect(actionAddNewMetadata, &QAction::triggered, this, &YaraViewMenu::onActionAddNewMetadata);
    connect(actionCopyName, &QAction::triggered, this, &YaraViewMenu::onActionCopyName);
    connect(actionSeekAt, &QAction::triggered, this, &YaraViewMenu::onActionSeekAt);
    connect(actionRemove, &QAction::triggered, this, &YaraViewMenu::onActionRemove);
    connect(actionRemoveAll, &QAction::triggered, this, &YaraViewMenu::onActionRemoveAll);

    addAction(actionAddNewMetadata);
    addAction(actionCopyName);
    addAction(actionSeekAt);
    addSeparator();
    addAction(actionRemove);
    addAction(actionRemoveAll);

    this->actionAddNewMetadata->setVisible(false);
    this->actionCopyName->setVisible(true);
    this->actionRemove->setVisible(true);
    this->actionRemoveAll->setVisible(true);
    this->actionSeekAt->setVisible(true);

    this->actionAddNewMetadata->setEnabled(true);
    this->actionCopyName->setEnabled(false);
    this->actionRemove->setEnabled(false);
    this->actionRemoveAll->setEnabled(false);
    this->actionSeekAt->setEnabled(false);
}

void YaraViewMenu::setYaraTarget(const YaraDescription &description, bool remove)
{
    this->target_yara = description;
    actionSeekAt->setText(tr("Seek At %1").arg(this->target_yara.name));
    actionRemove->setText(tr("Remove %1").arg(this->target_yara.name));

    this->hasYaraTarget = true;
    this->hasMetaTarget = false;
    this->actionAddNewMetadata->setVisible(false);
    this->actionCopyName->setVisible(true);
    this->actionSeekAt->setVisible(true);
    this->actionRemove->setVisible(remove);
    this->actionRemoveAll->setVisible(remove);

    bool is_valid = !this->target_yara.name.isEmpty();

    this->actionCopyName->setEnabled(is_valid);
    this->actionRemove->setEnabled(is_valid);
    this->actionRemoveAll->setEnabled(true);
    this->actionSeekAt->setEnabled(is_valid);
}

void YaraViewMenu::setMetaTarget(const MetadataDescription &description)
{
    this->target_meta = description;
    actionRemove->setText(tr("Remove %1").arg(this->target_meta.name));

    this->hasYaraTarget = false;
    this->hasMetaTarget = true;
    this->actionAddNewMetadata->setVisible(true);
    this->actionCopyName->setVisible(false);
    this->actionSeekAt->setVisible(false);
    this->actionRemove->setVisible(true);
    this->actionRemoveAll->setVisible(false);

    this->actionAddNewMetadata->setEnabled(true);
    this->actionRemove->setEnabled(!this->target_meta.name.isEmpty());
}

void YaraViewMenu::clearTarget()
{
    this->hasYaraTarget = false;
    this->hasMetaTarget = false;
    this->actionCopyName->setEnabled(false);
    this->actionSeekAt->setEnabled(false);
    this->actionRemove->setEnabled(false);
    this->actionRemoveAll->setEnabled(false);
    this->target_yara.name = "";
    this->target_meta.name = "";
}

void YaraViewMenu::onActionAddNewMetadata()
{
    YaraAddMetaDialog dialog(this);
    if (dialog.exec()) {
        emit Core()->yaraStringsChanged();
    }
}

void YaraViewMenu::onActionCopyName()
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(this->target_yara.name);
}

void YaraViewMenu::onActionSeekAt()
{
    if (this->hasYaraTarget) {
        bool is_va = Core()->getConfigb("io.va");
        bool is_va_match = this->target_yara.name.startsWith("yara.match.va.");
        bool is_pa_match = this->target_yara.name.startsWith("yara.match.va.");
        bool is_rule = this->target_yara.name.startsWith("yara.rule.");
        if (is_rule || (is_va && is_va_match) || (!is_va && is_pa_match)) {
            Core()->seek(this->target_yara.offset);
        } else {
            QMessageBox::warning(nullptr, tr("Yara"),
                                 tr("Cannot seek to %1 because the address is not mapped.")
                                         .arg(this->target_yara.name));
        }
    }
}

void YaraViewMenu::onActionRemove()
{
    if (this->hasYaraTarget) {
        Core()->cmd("yarasr " + this->target_yara.name);
        emit Core()->refreshCodeViews();
    } else if (this->hasMetaTarget) {
        Core()->cmd("yaramr " + this->target_meta.name);
    }
    emit Core()->yaraStringsChanged();
}

void YaraViewMenu::onActionRemoveAll()
{
    Core()->cmd("yarasc");
    emit Core()->refreshCodeViews();
    emit Core()->yaraStringsChanged();
}
