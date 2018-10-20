
#include "QuickFilterView.h"
#include "ui_QuickFilterView.h"

QuickFilterView::QuickFilterView(QWidget *parent, bool defaultOn) :
    QWidget(parent),
    ui(new Ui::QuickFilterView())
{
    ui->setupUi(this);

    connect(ui->closeFilterButton, &QAbstractButton::clicked, this, &QuickFilterView::closeFilter);

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this, [this](const QString & text) {
        emit filterTextChanged(text);
    });

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
