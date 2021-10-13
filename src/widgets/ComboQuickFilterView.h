#ifndef COMBOQUICKFILTERVIEW_H
#define COMBOQUICKFILTERVIEW_H

#include "core/CutterCommon.h"

#include <QWidget>
#include <QComboBox>

namespace Ui {
class ComboQuickFilterView;
}

class CUTTER_EXPORT ComboQuickFilterView : public QWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ComboQuickFilterView)                                                   \
        ComboQuickFilterView(const ComboQuickFilterView &v) = delete;                              \
        ComboQuickFilterView &operator=(const ComboQuickFilterView &v) = delete;

#    define Q_DISABLE_MOVE(ComboQuickFilterView)                                                   \
        ComboQuickFilterView(ComboQuickFilterView &&v) = delete;                                   \
        ComboQuickFilterView &operator=(ComboQuickFilterView &&v) = delete;

#    define Q_DISABLE_COPY_MOVE(ComboQuickFilterView)                                              \
        Q_DISABLE_COPY(ComboQuickFilterView)                                                       \
        Q_DISABLE_MOVE(ComboQuickFilterView)
#endif

    Q_DISABLE_COPY_MOVE(ComboQuickFilterView)

public:
    explicit ComboQuickFilterView(QWidget *parent = nullptr);
    ~ComboQuickFilterView() override;

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
    std::unique_ptr<Ui::ComboQuickFilterView> ui;
};

#endif // COMBOQUICKFILTERVIEW_H
