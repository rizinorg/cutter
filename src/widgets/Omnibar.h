#ifndef OMNIBAR_H
#define OMNIBAR_H

#include <QLineEdit>
#include <QCompleter>

class MainWindow;
class QStandardItemModel;

class OmnibarCompleter : public QCompleter
{
public:
    explicit OmnibarCompleter(QObject *parent = nullptr);

    /**
     * @brief Force the completer to match everything, since we are handling filtering on our own
     */
    QStringList splitPath(const QString &) const override;

    /**
     * @brief Do not auto-complete when selecting "Show More" or "Show All" entries
     *
     * Keeps the text the same for "Show More" and "Show All", auto-completes for other entries
     */
    QString pathFromIndex(const QModelIndex &index) const override;
};

class Omnibar : public QLineEdit
{
    Q_OBJECT
public:
    explicit Omnibar(MainWindow *main, QWidget *parent = nullptr);

    void refresh();

    enum ItemType { Standard = 0, ShowMore = 1, ShowAll = 2 };

private slots:
    /**
     * @brief Seeks to the address of selected entry/flag and shows it in the last memory widget
     */
    void goToEntry();

    /**
     * @brief Filters or searches entries "containing" the text
     *
     * Appends "Show More" and "Show All" entries at the end if there are more found matches then
     * currently shown
     *
     * @param text The string to match against
     * @param append If false, resets the search to the beginning. If true, continues from the last
     * found position
     */
    void handleSearch(const QString &text, bool append = false);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void clear();

private:
    void setupCompleter();
    void handleShowMore(int currentRow);
    void handleShowAll(int currentRow);

    MainWindow *main;
    QStringList m_flags;

    QStringList m_filteredFlags;
    QStandardItemModel *m_completerModel;
    QCompleter *m_completer;

    QString m_searchedText;
    int m_maxShownItems;
    int m_lastIndex;
    int m_matchCount;
};

#endif // OMNIBAR_H
