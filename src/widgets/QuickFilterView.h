#ifndef QUICKFILTERVIEW_H
#define QUICKFILTERVIEW_H

#include "AbstractFilterView.h"

#include <memory>

namespace Ui {
class QuickFilterView;
}

class QuickFilterView : public AbstractFilterView
{
    Q_OBJECT

public:
    explicit QuickFilterView(QWidget *parent = nullptr);
    ~QuickFilterView();

    void clearFilter();

protected:
    ItemCountLineEdit *lineEdit() const override;

private:
    std::unique_ptr<Ui::QuickFilterView> ui;
};

#endif // QUICKFILTERVIEW_H
