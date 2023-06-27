
#ifndef QUICKFILTERVIEW_H
#define QUICKFILTERVIEW_H

#include "core/CutterCommon.h"

#include <memory>

#include <QWidget>
#include <QTimer>

namespace Ui {
class QuickFilterView;
}

class CUTTER_EXPORT QuickFilterView : public QWidget
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
    QTimer *debounceTimer;
};

#endif // QUICKFILTERVIEW_H
