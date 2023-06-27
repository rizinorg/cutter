
#include "QuickFilterView.h"
#include "ui_QuickFilterView.h"

QuickFilterView::QuickFilterView(QWidget *parent, bool defaultOn)
    : QWidget(parent), ui(new Ui::QuickFilterView())
{
    ui->setupUi(this);

    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);

    connect(ui->closeFilterButton, &QAbstractButton::clicked, this, &QuickFilterView::closeFilter);

    connect(debounceTimer, &QTimer::timeout, this,
            [this]() { emit filterTextChanged(ui->filterLineEdit->text()); });

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this,
            [this]() { debounceTimer->start(150); });

    if (!defaultOn) {
        closeFilter();
    }
}

QuickFilterView::~QuickFilterView() {}

void QuickFilterView::showFilter()
{
    show();
    ui->filterLineEdit->setFocus();
}

void QuickFilterView::clearFilter()
{
    if (ui->filterLineEdit->text().isEmpty()) {
        closeFilter();
    } else {
        ui->filterLineEdit->setText("");
    }
}

void QuickFilterView::closeFilter()
{
    ui->filterLineEdit->setText("");
    hide();
    emit filterClosed();
}
