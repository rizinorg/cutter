#include "EditRegProfileDialog.h"
#include "ui_EditRegProfileDialog.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QMessageBox>
#include <QMenu>

EditRegProfileDialog::EditRegProfileDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::EditRegProfileDialog)
{
    ui->setupUi(this);

    aliasModel = new QStandardItemModel(0, ALIAS_MAX_COL, this);
    aliasModel->setHorizontalHeaderLabels({ tr("Alias"), tr("Register") });
    ui->aliasTable->setModel(aliasModel);

    regModel = new QStandardItemModel(0, REG_MAX_COL, this);
    regModel->setHorizontalHeaderLabels({ tr("Type"), tr("Name"), tr("Size"), tr("Offset"),
                                          tr("Packed"), tr("Flags/Comment") });
    ui->regTable->setModel(regModel);

    ui->aliasTable->verticalHeader()->hide();
    ui->regTable->verticalHeader()->hide();
    ui->aliasTable->horizontalHeader()->setStretchLastSection(true);
    ui->regTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->addRowBtn, &QPushButton::clicked, this, &EditRegProfileDialog::onAddRow);
    connect(ui->removeRowBtn, &QPushButton::clicked, this, &EditRegProfileDialog::onRemoveRow);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &EditRegProfileDialog::onTabChanged);

    ui->aliasTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->regTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->aliasTable, &QTableView::customContextMenuRequested, this,
            &EditRegProfileDialog::showContextMenu);
    connect(ui->regTable, &QTableView::customContextMenuRequested, this,
            &EditRegProfileDialog::showContextMenu);
}

EditRegProfileDialog::~EditRegProfileDialog() {}

EditRegProfileDialog::TabContext EditRegProfileDialog::getActiveTab() const
{
    if (ui->tabWidget->currentIndex() == TAB_ALIASES) {
        return { ui->aliasTable, aliasModel };
    }
    return { ui->regTable, regModel };
}

void EditRegProfileDialog::setProfileData(const QString &data)
{
    ui->rawProfileEdit->setPlainText(data);
    parseToTables(data);
}

QString EditRegProfileDialog::getProfleData() const
{
    if (ui->tabWidget->currentIndex() == TAB_RAW) {
        return ui->rawProfileEdit->toPlainText();
    }
    return tablesToText();
}

void EditRegProfileDialog::parseToTables(const QString &data)
{
    aliasModel->removeRows(0, aliasModel->rowCount());
    regModel->removeRows(0, regModel->rowCount());
    ui->aliasTable->clearSpans();
    ui->regTable->clearSpans();

    static const QRegularExpression re("\\s+");
    const QStringList lines = data.split('\n');

    bool parsingRegisters = false;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        if (!parsingRegisters && !trimmed.startsWith('#') && !trimmed.startsWith('=')) {
            parsingRegisters = true;
        }

        // handle comments
        if (trimmed.startsWith('#')) {
            QStandardItem *commentItem = new QStandardItem(trimmed);
            commentItem->setData(true, CommentRole);

            if (parsingRegisters) {
                QList<QStandardItem *> items = { commentItem };
                for (int i = 1; i < REG_MAX_COL; ++i) {
                    items << new QStandardItem("");
                }
                int rowIdx = regModel->rowCount();
                regModel->appendRow(items);
                ui->regTable->setSpan(rowIdx, 0, 1, REG_MAX_COL);
            } else {
                QList<QStandardItem *> items = { commentItem, new QStandardItem("") };
                int rowIdx = aliasModel->rowCount();
                aliasModel->appendRow(items);
                ui->aliasTable->setSpan(rowIdx, 0, 1, ALIAS_MAX_COL);
            }
            continue;
        }

        if (trimmed.startsWith('=')) {
            QStringList parts = trimmed.mid(1).split(re);
            QList<QStandardItem *> items;
            for (int i = 0; i < ALIAS_MAX_COL; ++i) {
                items << new QStandardItem(i < parts.size() ? parts[i] : "");
            }
            aliasModel->appendRow(items);
        } else {
            QStringList parts = trimmed.split(re);
            QList<QStandardItem *> items;
            for (int i = 0; i < REG_MAX_COL; ++i) {
                items << new QStandardItem(i < parts.size() ? parts[i] : "");
            }
            regModel->appendRow(items);
        }
    }
}

QString EditRegProfileDialog::tablesToText() const
{
    QString out;
    QTextStream ts(&out);

    // Aliases
    for (int i = 0; i < aliasModel->rowCount(); ++i) {
        QStandardItem *firstItem = aliasModel->item(i, 0);
        if (firstItem && firstItem->data(CommentRole).toBool()) {
            ts << firstItem->text() << "\n";
        } else {
            ts << "=";
            for (int j = 0; j < ALIAS_MAX_COL; ++j) {
                ts << aliasModel->index(i, j).data().toString().trimmed();
                if (j < ALIAS_MAX_COL - 1) {
                    ts << "\t";
                }
            }
            ts << "\n";
        }
    }

    // Registers
    for (int i = 0; i < regModel->rowCount(); ++i) {
        QStandardItem *firstItem = regModel->item(i, 0);
        if (firstItem && firstItem->data(CommentRole).toBool()) {
            ts << firstItem->text() << "\n";
        } else {
            for (int j = 0; j < REG_MAX_COL; ++j) {
                ts << regModel->index(i, j).data().toString().trimmed();
                if (j < REG_MAX_COL - 1) {
                    ts << "\t";
                }
            }
            ts << "\n";
        }
    }
    return out;
}

void EditRegProfileDialog::onAddRow()
{
    TabContext tabCtx = getActiveTab();
    auto model = tabCtx.model;
    auto view = tabCtx.view;
    QModelIndex current = view->currentIndex();
    int row = current.isValid() ? current.row() + 1 : model->rowCount();
    model->insertRow(row);

    QModelIndex newIdx = model->index(row, 0);
    view->setCurrentIndex(newIdx);
    view->edit(newIdx);
}

void EditRegProfileDialog::onRemoveRow()
{
    TabContext tabCtx = getActiveTab();
    auto model = tabCtx.model;
    auto view = tabCtx.view;
    QModelIndex current = view->currentIndex();
    if (current.isValid()) {
        model->removeRow(current.row());
    }
}

void EditRegProfileDialog::onTabChanged(int index)
{
    bool raw = (index == TAB_RAW);
    if (raw) {
        ui->rawProfileEdit->setPlainText(tablesToText());
    } else {
        parseToTables(ui->rawProfileEdit->toPlainText());
    }
    ui->addRowBtn->setVisible(!raw);
    ui->removeRowBtn->setVisible(!raw);
}

void EditRegProfileDialog::showContextMenu(const QPoint &pos)
{
    TabContext tabCtx = getActiveTab();
    auto view = tabCtx.view;
    auto model = tabCtx.model;

    QModelIndex index = view->indexAt(pos);
    int row = index.isValid() ? index.row() : model->rowCount();

    QMenu menu(this);
    QAction *addAbove = menu.addAction(tr("Add Row Above"));
    QAction *addBelow = menu.addAction(tr("Add Row Below"));
    menu.addSeparator();
    QAction *remove = menu.addAction(tr("Remove Row"));

    remove->setEnabled(index.isValid());
    QAction *selected = menu.exec(view->viewport()->mapToGlobal(pos));

    if (!selected) {
        return;
    }
    if (selected == addAbove) {
        model->insertRow(row);
        view->setCurrentIndex(model->index(row, 0));
    } else if (selected == addBelow) {
        model->insertRow(row + 1);
        view->setCurrentIndex(model->index(row + 1, 0));
    } else if (selected == remove) {
        model->removeRow(row);
    }
}
