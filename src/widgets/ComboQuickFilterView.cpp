#include "ComboQuickFilterView.h"
#include "ui_ComboQuickFilterView.h"

ComboQuickFilterView::ComboQuickFilterView(QWidget *parent)
    : AbstractFilterView(parent), ui(new Ui::ComboQuickFilterView)
{
    ui->setupUi(this);

    setupSharedConnections();

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ComboQuickFilterView::customContextMenuRequested, this,
            &ComboQuickFilterView::showCustomContextMenu);
}

ComboQuickFilterView::~ComboQuickFilterView() {}

ItemCountLineEdit *ComboQuickFilterView::lineEdit() const
{
    return ui->lineEdit;
}

void ComboQuickFilterView::setLabelText(const QString &text)
{
    ui->label->setText(text);
}

QComboBox *ComboQuickFilterView::comboBox()
{
    return ui->comboBox;
}
