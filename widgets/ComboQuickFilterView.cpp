#include "ComboQuickFilterView.h"
#include "ui_ComboQuickFilterView.h"

ComboQuickFilterView::ComboQuickFilterView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ComboQuickFilterView)
{
    ui->setupUi(this);

    connect(ui->lineEdit, &QLineEdit::textChanged, this, [this](const QString & text) {
        emit filterTextChanged(text);
    });
}

ComboQuickFilterView::~ComboQuickFilterView()
{
    delete ui;
}

void ComboQuickFilterView::setLabelText(const QString &text)
{
    ui->label->setText(text);
}

QComboBox *ComboQuickFilterView::comboBox()
{
    return ui->comboBox;
}

void ComboQuickFilterView::showFilter()
{
    show();
    ui->lineEdit->setFocus();
}

void ComboQuickFilterView::clearFilter()
{
    ui->lineEdit->setText("");
}

void ComboQuickFilterView::closeFilter()
{
    ui->lineEdit->setText("");
    hide();
    emit filterClosed();
}
