#ifndef COMBOQUICKFILTERVIEW_H
#define COMBOQUICKFILTERVIEW_H

#include <QWidget>
#include <QComboBox>

namespace Ui {
class ComboQuickFilterView;
}

class ComboQuickFilterView : public QWidget
{
    Q_OBJECT

public:
    explicit ComboQuickFilterView(QWidget *parent = nullptr);
    ~ComboQuickFilterView();

    void setLabelText(const QString &text);
    QComboBox *comboBox();

public slots:
    void showFilter();
    void closeFilter();
    void clearFilter();

signals:
    void filterTextChanged(const QString &text);
    void filterClosed();

private:
    Ui::ComboQuickFilterView *ui;
};

#endif // COMBOQUICKFILTERVIEW_H
