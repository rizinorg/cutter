#include "QuickFilterView.h"

#include "Configuration.h"
#include "ui_QuickFilterView.h"

QuickFilterView::QuickFilterView(QWidget *parent)
    : AbstractFilterView(parent), ui(new Ui::QuickFilterView())
{
    ui->setupUi(this);

    setupSharedConnections();

    connect(ui->closeFilterButton, &QAbstractButton::clicked, this, &QuickFilterView::closeFilter);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QuickFilterView::customContextMenuRequested, this,
            &QuickFilterView::showCustomContextMenu);

    connect(Config(), &Configuration::quickFilterOptionsChanged, this, [this]() {
        if (Config()->getShowQuickFilter()) {
            this->show();
        } else if (ui->filterLineEdit->text().isEmpty()) {
            this->hide();
        }
    });

    if (!Config()->getShowQuickFilter()) {
        hide();
    }
}

QuickFilterView::~QuickFilterView() {}

ItemCountLineEdit *QuickFilterView::lineEdit() const
{
    return ui->filterLineEdit;
}

void QuickFilterView::clearFilter()
{
    if (ui->filterLineEdit->text().isEmpty()) {
        closeFilter();
    } else {
        ui->filterLineEdit->setText("");
    }
}
