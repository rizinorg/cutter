#ifndef SEARCHABLETEXTEDIT_H
#define SEARCHABLETEXTEDIT_H

#include <QPlainTextEdit>

/**
 * @brief A text editor that adds search functionality
 * Use this instead of QPlainTextEdit in widgets that have a search bar
 */
class SearchableTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit SearchableTextEdit(QWidget *parent = nullptr);

    /**
     * @brief Search for a given string or regex in the document, selects the
     * first match on or after the cursor
     * @param string String or regex to search
     * @param flags Find flags
     * @param isRegex True if regex, false otherwise
     * @return <index, count> where index is currently selected index and count is total number of
     * matches
     */
    QPair<int, int> search(const QString &string, int options);

    /** returns new index */
    int findNext();
    int findPrev();
    int findLast();

    void clearSearch();

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void highlightMatches();

private:
    struct SearchResult
    {
        int position;
        int length;
    };

    int m_currentIndex;
    QList<SearchResult> m_searchResults;
    bool m_highlightMatches;

    void handleMatch(const QTextCursor &currentCursor, const QTextCursor &originalCursor);
    void scrollToCurrentIndex();

    inline void mapCursorToResult(QTextCursor &cursor, const SearchResult result)
    {
        cursor.setPosition(result.position);
        cursor.setPosition(result.position + result.length, QTextCursor::KeepAnchor);
    }
};

#endif // SEARCHABLETEXTEDIT_H
