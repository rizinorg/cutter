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
    void clearFilter() const;

    virtual void closeFilter();

    /**
     * @brief Sets up Find and Esc for this filter bar.
     *
     * Find opens the bar, Esc clears it, then closes it. If there's nothing to clear, Esc is left
     * alone so dialogs can still close.
     *
     * @param list The view the bar filters; gets focus back when the bar closes.
     * @param scope Where the keys work, besides the bar. Defaults to @p list.
     */
    void attachTo(QAbstractItemView *list, QWidget *scope = nullptr);

    /**
     * @brief Whether Esc (or a different shortcut) on an empty bar hides it.
     */
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
