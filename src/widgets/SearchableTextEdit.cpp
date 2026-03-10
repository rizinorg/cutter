#include "SearchableTextEdit.h"
#include "CutterSearchable.h"
#include "Configuration.h"

#include <QRegularExpression>

SearchableTextEdit::SearchableTextEdit(QWidget *parent) : QPlainTextEdit(parent) {}

QPair<int, int> SearchableTextEdit::search(const QString &string, int options)
{
    m_searchCursors.clear();
    m_currentIndex = -1;
    m_searchedMatchLen = -1;
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
            handleMatch(cursor, originalCursor);
        }
    } else {
        while (!cursor.isNull() && !cursor.atEnd()) {
            cursor = doc->find(string, cursor, flags);
            handleMatch(cursor, originalCursor);
        }
    }

    // wrap around
    if (m_currentIndex < 0 && !m_searchCursors.isEmpty()) {
        m_currentIndex = 0;
    }

    scrollToCurrentIndex();
    highlightMatches();

    return { m_currentIndex, m_searchCursors.size() };
}

void SearchableTextEdit::handleMatch(const QTextCursor &currentCursor,
                                     const QTextCursor &originalCursor)
{
    if (!currentCursor.isNull()) {

        if (m_currentIndex < 0 && currentCursor.selectionEnd() >= originalCursor.selectionStart()) {
            m_currentIndex = m_searchCursors.size();
        }

        m_searchCursors.append(currentCursor);
    }
}

void SearchableTextEdit::clearSearch()
{
    this->setExtraSelections({});
    m_searchCursors.clear();
}

int SearchableTextEdit::findNext()
{
    m_currentIndex = (m_currentIndex + 1) % m_searchCursors.size();

    scrollToCurrentIndex();
    highlightMatches();

    return m_currentIndex;
}

int SearchableTextEdit::findPrev()
{
    int count = m_searchCursors.size();
    m_currentIndex = (m_currentIndex - 1 + count) % count;
    scrollToCurrentIndex();
    highlightMatches();

    return m_currentIndex;
}

int SearchableTextEdit::findLast()
{
    m_currentIndex = m_searchCursors.size() - 1;

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
    if (m_currentIndex < 0 || m_currentIndex >= m_searchCursors.size()) {
        this->setExtraSelections({});
        return;
    }

    if (!m_highlightMatches) {
        const QTextCursor cursor = m_searchCursors[m_currentIndex];
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(ConfigColor("searchCurrent"));
        selection.cursor = cursor;
        this->setExtraSelections({ selection });
        return;
    }

    QPoint startPoint = QPoint(0, 0);
    int startPos = this->cursorForPosition(startPoint).position();
    startPos -= m_searchedMatchLen;
    startPos = startPos >= 0 ? startPos : 0;

    QPoint endPoint = QPoint(this->geometry().width(), this->geometry().height());
    auto endCursor = this->cursorForPosition(endPoint);
    int endPos = endCursor.position();
    endPos += m_searchedMatchLen;
    endCursor.movePosition(QTextCursor::End);
    endPos = endPos <= endCursor.position() ? endPos : endCursor.position();

    QList<QTextEdit::ExtraSelection> selections;
    for (int i = 0; i < m_searchCursors.size(); ++i) {
        const QTextCursor cursor = m_searchCursors[i];
        int pos = cursor.selectionStart();
        if (i == m_currentIndex) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(ConfigColor("searchCurrent"));
            selection.cursor = cursor;
            selections.append(selection);
        } else if (pos >= startPos && pos < endPos) {
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
    if (m_currentIndex < 0 || m_currentIndex >= m_searchCursors.size()) {
        return;
    }

    QTextCursor scrollCursor = m_searchCursors[m_currentIndex];
    scrollCursor.setPosition(scrollCursor.selectionStart());
    scrollCursor.clearSelection();
    this->setTextCursor(scrollCursor);
}
