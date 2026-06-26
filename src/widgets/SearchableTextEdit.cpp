#include "SearchableTextEdit.h"

#include "Configuration.h"
#include "CutterSearchable.h"

#include <QRegularExpression>

SearchableTextEdit::SearchableTextEdit(QWidget *parent) : QPlainTextEdit(parent) {}

QPair<int, int> SearchableTextEdit::search(const QString &string, int options)
{
    searchResults.clear();
    currentIndex = -1;
    highlightMatchesEnabled = options & SearchOption::HighlightMatches;

    if (string.isEmpty()) {
        clearSearch();
        return { 0, 0 };
    }

    QTextDocument::FindFlags flags = {};
    flags = options & SearchOption::CaseSensitive ? (flags | QTextDocument::FindCaseSensitively)
                                                  : flags;
    flags = options & SearchOption::WholeWords ? (flags | QTextDocument::FindWholeWords) : flags;

    // we will search relative to this cursor
    const QTextCursor originalCursor = this->textCursor();

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
    if (currentIndex < 0 && !searchResults.isEmpty()) {
        currentIndex = 0;
    }

    scrollToCurrentIndex();
    highlightMatches();

    return { currentIndex, searchResults.size() };
}

void SearchableTextEdit::handleMatch(const QTextCursor &currentCursor,
                                     const QTextCursor &originalCursor)
{
    if (!currentCursor.isNull()) {

        if (currentIndex < 0 && currentCursor.selectionEnd() >= originalCursor.selectionStart()) {
            currentIndex = searchResults.size();
        }

        const int len = currentCursor.selectionEnd() - currentCursor.selectionStart();
        searchResults.append(SearchResult { currentCursor.selectionStart(), len });
    }
}

void SearchableTextEdit::clearSearch()
{
    this->setExtraSelections({});
    searchResults.clear();
}

int SearchableTextEdit::findNext()
{
    if (searchResults.isEmpty()) {
        return 0;
    }

    currentIndex = (currentIndex + 1) % searchResults.size();

    scrollToCurrentIndex();
    highlightMatches();

    return currentIndex;
}

int SearchableTextEdit::findPrev()
{
    if (searchResults.isEmpty()) {
        return 0;
    }

    const int count = searchResults.size();
    currentIndex = (currentIndex - 1 + count) % count;
    scrollToCurrentIndex();
    highlightMatches();

    return currentIndex;
}

int SearchableTextEdit::findLast()
{
    if (searchResults.isEmpty()) {
        return 0;
    }

    currentIndex = searchResults.size() - 1;

    scrollToCurrentIndex();
    highlightMatches();

    return currentIndex;
}

void SearchableTextEdit::resizeEvent(QResizeEvent *event)
{
    highlightMatches();
    QPlainTextEdit::resizeEvent(event);
}

void SearchableTextEdit::highlightMatches()
{
    if (currentIndex < 0 || currentIndex >= searchResults.size()) {
        this->setExtraSelections({});
        return;
    }

    QTextCursor cursor(this->document());

    if (!highlightMatchesEnabled) {
        mapCursorToResult(cursor, searchResults[currentIndex]);
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(ConfigColor("searchCurrent"));
        selection.cursor = cursor;
        this->setExtraSelections({ selection });
        return;
    }

    const QPoint startPoint = QPoint(0, 0);
    const int startPos = this->cursorForPosition(startPoint).position();

    const QPoint endPoint = QPoint(this->geometry().width(), this->geometry().height());
    auto endCursor = this->cursorForPosition(endPoint);
    const int endPos = endCursor.position();
    endCursor.movePosition(QTextCursor::End);
    const int maxPos = endCursor.position();

    QList<QTextEdit::ExtraSelection> selections;
    for (int i = 0; i < searchResults.size(); ++i) {
        const auto res = searchResults[i];
        mapCursorToResult(cursor, searchResults[i]);
        const int sPos = std::max(startPos - res.length, 0);
        const int ePos = std::min(endPos + res.length, maxPos);
        if (i == currentIndex) {
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
    if (currentIndex < 0 || currentIndex >= searchResults.size()) {
        return;
    }

    QTextCursor scrollCursor(this->document());
    mapCursorToResult(scrollCursor, searchResults[currentIndex]);
    scrollCursor.setPosition(scrollCursor.selectionStart());
    scrollCursor.clearSelection();
    this->setTextCursor(scrollCursor);
}
