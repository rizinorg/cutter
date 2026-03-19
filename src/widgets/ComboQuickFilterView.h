#ifndef COMBOQUICKFILTERVIEW_H
#define COMBOQUICKFILTERVIEW_H

#include "AbstractFilterView.h"
#include <QComboBox>
#include <memory>

namespace Ui {
class ComboQuickFilterView;
}

class ComboQuickFilterView : public AbstractFilterView
{
    Q_OBJECT

public:
    explicit ComboQuickFilterView(QWidget *parent = nullptr);
    ~ComboQuickFilterView();

    void setLabelText(const QString &text);
    QComboBox *comboBox();

protected:
    ItemCountLineEdit *lineEdit() const override;

private:
    std::unique_ptr<Ui::ComboQuickFilterView> ui;
};

#endif // COMBOQUICKFILTERVIEW_H
