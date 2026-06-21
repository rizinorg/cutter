#include "StringListDialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

StringListDialog::StringListDialog(const QStringList &initialList, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit List"));
    auto *layout = new QVBoxLayout(this);
    auto *hl = new QHBoxLayout();

    listView = new QListWidget(this);
    for (const auto &str : initialList) {
        auto *item = new QListWidgetItem(str, listView);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    hl->addWidget(listView);

    auto *btnLayout = new QVBoxLayout();
    auto *addBtn = new QPushButton(tr("Add"), this);
    auto *removeBtn = new QPushButton(tr("Remove"), this);
    auto *removeAllBtn = new QPushButton(tr("Remove All"), this);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(removeAllBtn);
    btnLayout->addStretch();
    hl->addLayout(btnLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addLayout(hl);
    layout->addWidget(buttonBox);

    connect(addBtn, &QPushButton::clicked, this, [this]() {
        auto *item = new QListWidgetItem("item", listView);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        listView->setCurrentItem(item);
        listView->editItem(item);
    });
    connect(removeBtn, &QPushButton::clicked, this, [this]() { delete listView->currentItem(); });
    connect(removeAllBtn, &QPushButton::clicked, this, [this]() { listView->clear(); });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    listView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

QStringList StringListDialog::getStringList() const
{
    QStringList list;
    for (int i = 0; i < listView->count(); ++i) {
        list << listView->item(i)->text();
    }
    return list;
}
