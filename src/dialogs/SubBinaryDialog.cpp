#include "SubBinaryDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

SubBinaryDialog::SubBinaryDialog(const QList<SubBinaryDescription> &subBinaries, QWidget *parent)
    : QDialog(parent), buttonGroup(new QButtonGroup(this))
{
    setWindowTitle(tr("Select Binary"));
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(new QLabel(tr("This file contains multiple binaries:"), this));

    for (int i = 0; i < subBinaries.size(); ++i) {
        const auto &sub = subBinaries[i];
        QString label = sub.arch;
        if (!sub.machine.isEmpty() && sub.machine != "all" && sub.machine != sub.arch) {
            label = label.isEmpty() ? sub.machine : QString("%1 (%2)").arg(label, sub.machine);
        }
        if (label.isEmpty()) {
            label = tr("Binary #%1").arg(sub.index);
        }
        if (sub.bits > 0) {
            label += QString(" %1-bit").arg(sub.bits);
        }
        if (!sub.type.isEmpty()) {
            label += QString("  -  %1").arg(sub.type);
        }

        auto *radio = new QRadioButton(label, this);
        if (i == 0) {
            radio->setChecked(true);
        }
        buttonGroup->addButton(radio, sub.index);
        layout->addWidget(radio);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Open, this);
    if (auto *openBtn = buttons->button(QDialogButtonBox::Open)) {
        openBtn->setDefault(true);
    }
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    setMinimumWidth(340);
}

int SubBinaryDialog::selectedIndex() const
{
    return buttonGroup->checkedId();
}
