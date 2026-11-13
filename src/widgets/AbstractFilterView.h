#ifndef ABSTRACTFILTERVIEW_H
#define ABSTRACTFILTERVIEW_H

#include "ItemCountLineEdit.h"

#include <QTimer>
#include <QWidget>

/**
 * @brief Base class for filter widgets providing a debounced search interface
 * and item count display
 */
class AbstractFilterView : public QWidget
{
    Q_OBJECT
public:
    explicit AbstractFilterView(QWidget *parent = nullptr);

    virtual void setItemCount(int count);
    virtual void showItemCount(bool show);

    void showFilter();
    void clearFilter();

    virtual void closeFilter();

signals:
    void filterTextChanged(const QString &text);
    void filterClosed();

protected:
    virtual ItemCountLineEdit *lineEdit() const = 0;

    void setupSharedConnections();
    void showCustomContextMenu(const QPoint &pos);

    QTimer *debounceTimer;
};

#endif // ABSTRACTFILTERVIEW_H
