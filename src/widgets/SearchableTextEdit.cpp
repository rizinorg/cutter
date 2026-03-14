#include "SearchableTextEdit.h"
#include "CutterSearchable.h"
#include "Configuration.h"

#include <QRegularExpression>

SearchableTextEdit::SearchableTextEdit(QWidget *parent) : QPlainTextEdit(parent) {}

QPair<int, int> SearchableTextEdit::search(const QString &string, int options)
{
    m_searchResults.clear();
    m_currentIndex = -1;
    m_highlightMatches = options & SearchOption::HighlightMatches;

    if (string.isEmpty()) {
        clearSearch();
        return { 0, 0 };
    }

    QTextDocument::FindFlags flags = {};
    flags = options & SearchOption::CaseSensitive ? (flags | QTextDocument::FindCaseSensitively)
                                                  : flags;
    flags = options & SearchOption::WholeWords ? (flags | QTextDocument::FindWholeWords) : flags;

    // we will search relative to this cursor
    QTextCursor originalCursor = this->textCursor();

    // start from the top
    QTextDocument *doc = this->document();
    QTextCursor cursor(doc);

    if (options & SearchOption::RegExp) {
        const QRegularExpression regex(string);
        while (!cursor.isNull() && !cursor.atEnd()) {
            cursor = doc->find(regex, cursor, flags);

            if (cursor.hasSelection()) {
                handleMatch(cursor, originalCursor);
            } else {
                cursor.movePosition(QTextCursor::NextCharacter); // avoid infinite loop
            }
        }
    } else {
        while (!cursor.isNull() && !cursor.atEnd()) {
            cursor = doc->find(string, cursor, flags);
            handleMatch(cursor, originalCursor);
        }
    }

    // wrap around
    if (m_currentIndex < 0 && !m_searchResults.isEmpty()) {
        m_currentIndex = 0;
    }

    scrollToCurrentIndex();
    highlightMatches();

    return { m_currentIndex, m_searchResults.size() };
}

void SearchableTextEdit::handleMatch(const QTextCursor &currentCursor,
                                     const QTextCursor &originalCursor)
{
    if (!currentCursor.isNull()) {

        if (m_currentIndex < 0 && currentCursor.selectionEnd() >= originalCursor.selectionStart()) {
            m_currentIndex = m_searchResults.size();
        }

        int len = currentCursor.selectionEnd() - currentCursor.selectionStart();
        m_searchResults.append(SearchResult { currentCursor.selectionStart(), len });
    }
}

void SearchableTextEdit::clearSearch()
{
    this->setExtraSelections({});
    m_searchResults.clear();
}

int SearchableTextEdit::findNext()
{
    if (m_searchResults.isEmpty()) {
        return 0;
    }

    m_currentIndex = (m_currentIndex + 1) % m_searchResults.size();

    scrollToCurrentIndex();
    highlightMatches();

    return m_currentIndex;
}

int SearchableTextEdit::findPrev()
{
    if (m_searchResults.isEmpty()) {
        return 0;
    }

    int count = m_searchResults.size();
    m_currentIndex = (m_currentIndex - 1 + count) % count;
    scrollToCurrentIndex();
    highlightMatches();

    return m_currentIndex;
}

int SearchableTextEdit::findLast()
{
    if (m_searchResults.isEmpty()) {
        return 0;
    }

    m_currentIndex = m_searchResults.size() - 1;

    scrollToCurrentIndex();
    highlightMatches();

    return m_currentIndex;
}

void SearchableTextEdit::resizeEvent(QResizeEvent *event)
{
    highlightMatches();
    QPlainTextEdit::resizeEvent(event);
}

void SearchableTextEdit::highlightMatches()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_searchResults.size()) {
        this->setExtraSelections({});
        return;
    }

    QTextCursor cursor(this->document());

    if (!m_highlightMatches) {
        mapCursorToResult(cursor, m_searchResults[m_currentIndex]);
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(ConfigColor("searchCurrent"));
        selection.cursor = cursor;
        this->setExtraSelections({ selection });
        return;
    }

    QPoint startPoint = QPoint(0, 0);
    int startPos = this->cursorForPosition(startPoint).position();

    QPoint endPoint = QPoint(this->geometry().width(), this->geometry().height());
    auto endCursor = this->cursorForPosition(endPoint);
    int endPos = endCursor.position();
    endCursor.movePosition(QTextCursor::End);
    int maxPos = endCursor.position();

    QList<QTextEdit::ExtraSelection> selections;
    for (int i = 0; i < m_searchResults.size(); ++i) {
        const auto res = m_searchResults[i];
        mapCursorToResult(cursor, m_searchResults[i]);
        int sPos = std::max(startPos - res.length, 0);
        int ePos = std::min(endPos + res.length, maxPos);
        if (i == m_currentIndex) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(ConfigColor("searchCurrent"));
            selection.cursor = cursor;
            selections.append(selection);
        } else if (cursor.selectionStart() >= sPos && cursor.selectionEnd() <= ePos) {
            // only highlight visible matches
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(ConfigColor("searchHighlight"));
            selection.cursor = cursor;
            selections.append(selection);
        }
    }
    this->setExtraSelections(selections);
}

void SearchableTextEdit::scrollToCurrentIndex()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_searchResults.size()) {
        return;
    }

    QTextCursor scrollCursor(this->document());
    mapCursorToResult(scrollCursor, m_searchResults[m_currentIndex]);
    scrollCursor.setPosition(scrollCursor.selectionStart());
    scrollCursor.clearSelection();
    this->setTextCursor(scrollCursor);
}
