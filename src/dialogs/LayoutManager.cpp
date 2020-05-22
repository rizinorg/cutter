#include "LayoutManager.h"
#include "ui_LayoutManager.h"
#include <QIntValidator>
#include <QInputDialog>

using namespace Cutter;

LayoutManager::LayoutManager(QMap<QString, Cutter::CutterLayout> &layouts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayoutManager),
    layouts(layouts)
{
    ui->setupUi(this);
    connect(ui->renameButton, &QPushButton::clicked, this, &LayoutManager::renameCurrentLayout);
    connect(ui->deleteButton, &QPushButton::clicked, this, &LayoutManager::deleteLayout);
    connect(ui->layoutSelector, &QComboBox::currentTextChanged, this, &LayoutManager::updateButtons);
    refreshNameList();
}

LayoutManager::~LayoutManager()
{
}

void LayoutManager::refreshNameList(QString selection)
{
    ui->layoutSelector->clear();
    for (auto it = layouts.begin(), end = layouts.end(); it != end; ++it) {
        if (!Cutter::isBuiltinLayoutName(it.key())) {
            ui->layoutSelector->addItem(it.key());
        }
    }
    if (!selection.isEmpty()) {
        ui->layoutSelector->setCurrentText(selection);
    }
    updateButtons();
}

void LayoutManager::renameCurrentLayout()
{
    QString current = ui->layoutSelector->currentText();
    if (layouts.contains(current)) {
        QString newName;
        while (newName.isEmpty() || isBuiltinLayoutName(newName) || layouts.contains(newName)) {
            if (!newName.isEmpty()) {
                QMessageBox::warning(this, tr("Rename layout error"), tr("'%1' is already used.").arg(newName));
            }
            newName = QInputDialog::getText(this, tr("Save layout"), tr("Enter name"), QLineEdit::Normal,
                                            current);
            if (newName.isEmpty()) {
                return;
            }
        }
        auto layout = layouts.take(current);
        layouts.insert(newName, layout);
        refreshNameList(newName);
    }
}

void LayoutManager::deleteLayout()
{
    auto selected = ui->layoutSelector->currentText();
    auto answer = QMessageBox::question(this, tr("Delete"),
                                        tr("Do you want to delete '%1'").arg(selected));
    if (answer == QMessageBox::Yes) {
        layouts.remove(selected);
        refreshNameList();
    }
}

void LayoutManager::updateButtons()
{
    bool hasSelection = !ui->layoutSelector->currentText().isEmpty();
    ui->renameButton->setEnabled(hasSelection);
    ui->deleteButton->setEnabled(hasSelection);
}
