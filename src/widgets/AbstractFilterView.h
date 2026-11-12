#ifndef ABSTRACTFILTERVIEW_H
#define ABSTRACTFILTERVIEW_H

#include "ItemCountLineEdit.h"

#include <QList>
#include <QTimer>
#include <QWidget>

class QAbstractItemView;
class QShortcut;

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

    /**
     * @brief Wires the Find and Esc keys for this filter bar and the list it filters.
     *
     * Find shows and focuses the bar. Esc clears its text, and on an empty bar closes it if
     * the bar can close, then returns focus to the list. Both keys are active while focus is
     * inside @p scope (the list itself if null) and inside the bar. Esc is left to the window
     * when the bar has nothing to clear or close, so a dialog can still be closed with it.
     * Call this once instead of creating the shortcuts by hand.
     */
    void attachTo(QAbstractItemView *list, QWidget *scope = nullptr);

    /** @brief Whether Esc on an empty bar hides it. */
    virtual bool isClosable() const { return false; }
    virtual ItemCountLineEdit *lineEdit() const = 0;

signals:
    void filterTextChanged(const QString &text);
    void filterClosed();

protected:
    void setupSharedConnections();
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showCustomContextMenu(const QPoint &pos);

    QTimer *debounceTimer;

private:
    void clearOrCloseFilter();
    void updateEscShortcuts();

    QList<QShortcut *> escShortcuts;
};

#endif // ABSTRACTFILTERVIEW_H
