
#ifndef QUICKFILTERVIEW_H
#define QUICKFILTERVIEW_H

#include <memory>

#include <QWidget>

namespace Ui {
class QuickFilterView;
}

class QuickFilterView : public QWidget
{
    Q_OBJECT

public:
    explicit QuickFilterView(QWidget *parent = nullptr, bool defaultOn = true);
    ~QuickFilterView();

public slots:
    void showFilter();
    void closeFilter();
    void clearFilter();

signals:
    void filterTextChanged(const QString &text);
    void filterClosed();

private:
    std::unique_ptr<Ui::QuickFilterView> ui;
};

#endif //QUICKFILTERVIEW_H
