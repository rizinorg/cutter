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
