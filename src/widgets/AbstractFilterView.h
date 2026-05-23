#ifndef ABSTRACTFILTERVIEW_H
#define ABSTRACTFILTERVIEW_H

#include "ItemCountLineEdit.h"

#include <QAction>
#include <QPoint>
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
    virtual ~AbstractFilterView() = default;

    virtual void setItemCount(int count);
    virtual void showItemCount(bool show);

    void showFilter();
    virtual void clearFilter();

    virtual void closeFilter();

signals:
    void filterTextChanged(const QString &text);
    void filterChanged(const QString &text, int options);
    void filterClosed();

protected:
    virtual ItemCountLineEdit *lineEdit() const = 0;

    void setupSharedConnections();
    void showCustomContextMenu(const QPoint &pos);

    int filterOptions() const;

    QTimer *debounceTimer = nullptr;

    QAction *caseSensitiveAction = nullptr;
    QAction *wholeWordsAction = nullptr;
    QAction *regexAction = nullptr;
};

#endif // ABSTRACTFILTERVIEW_H
