#include "DisassemblyWidget.h"

#include "DisassemblyHelper.h"
#include "common/BinaryTrees.h"
#include "common/Configuration.h"
#include "common/DisassemblyPreview.h"
#include "common/Helpers.h"
#include "common/SelectionHighlight.h"
#include "common/TempConfig.h"
#include "core/MainWindow.h"
#include "menus/DisassemblyContextMenu.h"
#include "shortcuts/ShortcutManager.h"
#include "widgets/AddressRangeScrollBar.h"

#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlockUserData>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace DisHlp = DisassemblyHelper;

DisassemblyWidget::DisassemblyWidget(MainWindow *main)
    : MemoryDockWidget(MemoryWidgetType::Disassembly, main),
      mCtxMenu(new DisassemblyContextMenu(this, main)),
      mDisasScrollArea(new DisassemblyScrollArea(this)),
      mDisasTextEdit(new DisassemblyTextEdit(this)),
      leftPanel(new DisassemblyLeftPanel(this)),
      maxLines(0),
      cursorLineOffset(0),
      cursorCharOffset(0),
      seekFromCursor(false),
      disasmRefresh(createReplacingRefreshDeferrer<RVA>(
              false, [this](const RVA *offset) { refreshDisasm(offset ? *offset : RVA_INVALID); }))
{
    setObjectName(main ? main->getUniqueObjectName(getWidgetType()) : getWidgetType());
    updateWindowTitle();

    topOffset = bottomOffset = RVA_INVALID;

    // Instantiate the window layout
    auto *splitter = new QSplitter;

    // Setup the left frame that contains breakpoints and jumps

    splitter->addWidget(leftPanel);

    // Setup the disassembly content
    auto *layout = new QHBoxLayout;
    layout->addWidget(mDisasTextEdit);
    layout->setContentsMargins(0, 0, 0, 0);
    mDisasScrollArea->viewport()->setLayout(layout);
    splitter->addWidget(mDisasScrollArea);
    connect(mDisasScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) { refreshDisasm(mDisasScrollArea->verticalScrollBar()->address()); });
    // Use stylesheet instead of QWidget::setFrameShape(QFrame::NoShape) to avoid
    // issues with dark and light interface themes
    mDisasScrollArea->setStyleSheet("QAbstractScrollArea { border: 0px transparent black; }");
    mDisasTextEdit->setStyleSheet("QPlainTextEdit { border: 0px transparent black; }");
    mDisasTextEdit->setFocusProxy(this);
    mDisasTextEdit->setFocusPolicy(Qt::ClickFocus);
    mDisasScrollArea->setFocusProxy(this);
    mDisasScrollArea->setFocusPolicy(Qt::ClickFocus);

    setFocusPolicy(Qt::ClickFocus);

    // Behave like all widgets: highlight on focus and hover
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now) {
        const QColor borderColor = this == now ? palette().color(QPalette::Highlight)
                                               : palette().color(QPalette::WindowText).darker();
        widget()->setStyleSheet(QString("QSplitter { border: %1px solid %2 } \n"
                                        "QSplitter:hover { border: %1px solid %3 } \n")
                                        .arg(devicePixelRatio())
                                        .arg(borderColor.name())
                                        .arg(palette().color(QPalette::Highlight).name()));
    });

    splitter->setFrameShape(QFrame::Box);
    // Set current widget to the splitted layout we just created
    setWidget(splitter);

    // Resize properly
    QList<int> sizes;
    sizes.append(3);
    sizes.append(1);
    splitter->setSizes(sizes);

    setAllowedAreas(Qt::AllDockWidgetAreas);

    setupFonts();
    setupColors();

    mDisasTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mDisasTextEdit->setFont(Config()->getFont());
    mDisasTextEdit->setReadOnly(true);
    mDisasTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    // wrapping breaks readCurrentDisassemblyOffset() at the moment :-(
    mDisasTextEdit->setWordWrapMode(QTextOption::NoWrap);

    // Increase asm text edit margin
    QTextDocument *asmDocu = mDisasTextEdit->document();
    asmDocu->setDocumentMargin(10);

    // Event filter to intercept double clicks in the textbox
    // and showing tooltips when hovering above those offsets
    mDisasTextEdit->viewport()->installEventFilter(this);

    // Set Disas context menu
    mDisasTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mDisasTextEdit, &QWidget::customContextMenuRequested, this,
            &DisassemblyWidget::showDisasContextMenu);

    connect(mDisasScrollArea, &DisassemblyScrollArea::scrollLines, this,
            &DisassemblyWidget::scrollInstructions);
    connect(mDisasScrollArea, &DisassemblyScrollArea::disassemblyResized, this,
            &DisassemblyWidget::updateMaxLines);
    connect(mDisasScrollArea, &DisassemblyScrollArea::wheelEventTriggered, this,
            &DisassemblyWidget::showTransientScrollBar);

    connectCursorPositionChanged(false);

    // Avoids the "The slot requires more arguments than the signal provides" error
    auto refresh = [this]() { refreshDisasm(); };
    connect(Core(), &CutterCore::commentsChanged, this, refresh);
    connect(Core(), &CutterCore::flagsChanged, this, refresh);
    connect(Core(), &CutterCore::globalVarsChanged, this, refresh);
    connect(Core(), &CutterCore::functionsChanged, this, refresh);
    connect(Core(), &CutterCore::functionRenamed, this, refresh);
    connect(Core(), &CutterCore::varsChanged, this, refresh);
    connect(Core(), &CutterCore::asmOptionsChanged, this, refresh);
    connect(Core(), &CutterCore::refreshCodeViews, this, [this] {
        breakpointsDirty = true;
        refreshDisasm();
    });
    connect(Core(), &CutterCore::instructionChanged, this, &DisassemblyWidget::instructionChanged);
    connect(Core(), &CutterCore::breakpointsChanged, this, [this](RVA offset) {
        breakpointsDirty = true;
        refreshIfInRange(offset);
    });

    connect(Config(), &Configuration::fontsUpdated, this, &DisassemblyWidget::fontsUpdatedSlot);
    connect(Config(), &Configuration::colorsUpdated, this, &DisassemblyWidget::colorsUpdatedSlot);

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        // just in case if breakpoints were changed via rizin console
        breakpointsDirty = true;
        refreshDisasm(seekable->getOffset());
    });
    refreshDisasm(seekable->getOffset());

    connect(mCtxMenu, &DisassemblyContextMenu::copy, this, &DisassemblyWidget::copySelection);

    mCtxMenu->addSeparator();
    mCtxMenu->addAction(&syncAction);
    connect(seekable, &CutterSeekable::seekableSeekChanged, this,
            &DisassemblyWidget::onSeekChanged);

    addActions(mCtxMenu->actions());

#define ADD_ACTION(id, ctx, slot)                                                                  \
    {                                                                                              \
        QAction *a = Shortcuts()->makeAction(id, this);                                            \
        a->setShortcutContext(ctx);                                                                \
        addAction(a);                                                                              \
        connect(a, &QAction::triggered, this, (slot));                                             \
    }

    // Space to switch to graph
    ADD_ACTION("Disassembly.switchToGraph", Qt::WidgetWithChildrenShortcut,
               [this] { mainWindow->showMemoryWidget(MemoryWidgetType::Graph); })

    ADD_ACTION("General.seekPrev", Qt::WidgetWithChildrenShortcut, &DisassemblyWidget::seekPrev)

    ADD_ACTION("Disassembly.pageUp", Qt::WidgetWithChildrenShortcut,
               [this] { moveCursorRelative(QTextCursor::Up, true); });

    ADD_ACTION("Disassembly.pageDown", Qt::WidgetWithChildrenShortcut,
               [this] { moveCursorRelative(QTextCursor::Down, true); });

#undef ADD_ACTION

    QTimer::singleShot(0, [this] { updateMaxLines(); });
    connect(this, &CutterDockWidget::becameVisibleToUser, this, [this] { updateMaxLines(); });
}

void DisassemblyWidget::setPreviewMode(bool previewMode)
{
    mDisasTextEdit->setContextMenuPolicy(previewMode ? Qt::NoContextMenu : Qt::CustomContextMenu);
    mCtxMenu->setEnabled(!previewMode);
    for (auto action : mCtxMenu->actions()) {
        action->setEnabled(!previewMode);
    }
    for (auto action : actions()) {
        if (action->shortcut() == Qt::Key_Space || action->shortcut() == Qt::Key_Escape) {
            action->setEnabled(!previewMode);
        }
    }
    if (previewMode) {
        seekable->setSynchronization(false);
    }
}

QWidget *DisassemblyWidget::getTextWidget()
{
    return mDisasTextEdit;
}

QString DisassemblyWidget::getWidgetType()
{
    return "Disassembly";
}

QFontMetricsF DisassemblyWidget::getFontMetrics()
{
    return QFontMetricsF(mDisasTextEdit->font());
}

QList<DisassemblyLine> DisassemblyWidget::getLines()
{
    return lines;
}

void DisassemblyWidget::showTransientScrollBar()
{
    mDisasScrollArea->verticalScrollBar()->showTransientScrollBar();
}

void DisassemblyWidget::refreshIfInRange(RVA offset)
{
    if (offset >= topOffset && offset <= bottomOffset) {
        refreshDisasm();
    }
}

void DisassemblyWidget::instructionChanged(RVA offset)
{
    leftPanel->clearArrowFrom(offset);
    refreshDisasm();
}

void DisassemblyWidget::refreshDisasm(RVA offset, RefreshMode mode)
{
    if (!disasmRefresh->attemptRefresh(offset == RVA_INVALID ? nullptr : new RVA(offset))) {
        return;
    }

    if (offset != RVA_INVALID) {
        topOffset = offset;
    }

    if (topOffset == RVA_INVALID) {
        return;
    }

    if (maxLines <= 0) {
        connectCursorPositionChanged(true);
        mDisasTextEdit->clear();
        connectCursorPositionChanged(false);
        return;
    }

    if (breakpointsDirty) {
        breakpoints = Core()->getBreakpointsAddresses();
        breakpointsDirty = false;
    }
    const int horizontalScrollValue = mDisasTextEdit->horizontalScrollBar()->value();
    mDisasTextEdit->setLockScroll(true);

    // Retrieve disassembly lines
    {
        TempConfig tempConfig;
        tempConfig.set("scr.color", COLOR_MODE_16M).set("asm.lines", false);

        if (mode == RefreshMode::Reset || lines.isEmpty()) {
            lines = Core()->disassembleLines(topOffset, maxLines);
            startIndex = 0;
        } else if (mode == RefreshMode::Append) {
            const RVA newOffset = Core()->nextOpAddr(lines.last().offset, 1);
            lines.append(Core()->disassembleLines(newOffset, maxLines));

        } else if (mode == RefreshMode::Prepend) {
            const RVA newOffset = Core()->prevOpAddr(lines.first().offset, maxLines);
            auto newLines = Core()->disassembleLines(newOffset, maxLines);

            // remove duplicates if any
            if (!newLines.isEmpty() && !lines.isEmpty()) {
                int keepSize = newLines.size();
                const RVA firstOffset = lines.first().offset;
                for (int i = newLines.size() - 1; i >= 0; --i) {
                    if (newLines[i].offset >= firstOffset) {
                        keepSize = i;
                    } else {
                        break;
                    }
                }
                if (keepSize < newLines.size()) {
                    newLines.erase(newLines.begin() + keepSize, newLines.end());
                }
            }

            const int prependCount = newLines.size();
            newLines.append(lines);
            lines = std::move(newLines);

            startIndex += prependCount;
        }
    }

    startIndex = qMax(0, qMin(startIndex, static_cast<int>(lines.size()) - 1));
    endIndex = qMin(lines.size(), startIndex + maxLines);

    // render lines
    connectCursorPositionChanged(true);
    mDisasTextEdit->document()->clear();
    QTextCursor cursor(mDisasTextEdit->document());
    cursor.beginEditBlock();

    const QTextBlockFormat regular = cursor.blockFormat();
    const QColor breakpointBg(ConfigColor("gui.breakpoint_background"));

    for (int i = startIndex; i < endIndex; ++i) {
        const auto &line = lines[i];

        cursor.insertHtml(line.text);
        if (Core()->isBreakpoint(breakpoints, line.offset)) {
            QTextBlockFormat f;
            f.setBackground(breakpointBg);
            cursor.setBlockFormat(f);
        }
        auto a = new DisassemblyTextBlockUserData(line);
        cursor.block().setUserData(a);
        cursor.insertBlock();
        cursor.setBlockFormat(regular);
    }

    cursor.endEditBlock();

    if (!lines.isEmpty()) {
        bottomOffset = lines[endIndex - 1].offset;
        if (bottomOffset < topOffset) {
            bottomOffset = RVA_MAX;
        }
    } else {
        bottomOffset = topOffset;
    }

    // update cursor position
    connectCursorPositionChanged(false);
    updateCursorPosition();
    connectCursorPositionChanged(true);

    // update cursor selection
    const bool hasSelection = (selectionAnchorRVA != RVA_INVALID && selectionPosRVA != RVA_INVALID
                               && (selectionAnchorRVA != selectionPosRVA
                                   || selectionAnchorSubIndex != selectionPosSubIndex
                                   || selectionAnchorChar != selectionPosChar));
    if (hasSelection) {
        updateSelection();
    }
    connectCursorPositionChanged(false);

    // update ctx menu
    updateContextMenuSelection(hasSelection);

    // update scrollbars
    mDisasTextEdit->setLockScroll(false);
    mDisasTextEdit->horizontalScrollBar()->setValue(horizontalScrollValue);
    mDisasTextEdit->verticalScrollBar()->setValue(0);

    mDisasScrollArea->verticalScrollBar()->setPosition(topOffset);

    // update left panel (trigger paint event)
    leftPanel->update();

    // update buffer by erasing extra lines
    constexpr int multiplier = 5;
    const int targetSize = maxLines * multiplier;
    if (lines.size() <= targetSize) {
        return;
    }

    // TODO: its fine for now but maybe in future this could be made more efficie
    // if the complexity trade-off is bearable
    const int idealKeepStart = startIndex - (targetSize - maxLines) / 2;
    const int keepStart =
            qMax(0, qMin(idealKeepStart, static_cast<int>(lines.size() - targetSize)));
    const int removeBottom = lines.size() - (keepStart + targetSize);
    if (removeBottom > 0) {
        lines.erase(lines.begin() + keepStart + targetSize, lines.begin() + lines.size());
    }
    if (keepStart > 0) {
        lines.erase(lines.begin(), lines.begin() + keepStart);
        startIndex -= keepStart;
    }
}

void DisassemblyWidget::scrollInstructions(int count, bool clampToScrollBarRange)
{
    if (count == 0) {
        return;
    }

    startIndex += count;
    RefreshMode mode = RefreshMode::Reset;
    if (startIndex < 0) {
        mode = RefreshMode::Prepend;
    } else if (startIndex + maxLines > lines.size()) {
        mode = RefreshMode::Append;
    } else if (startIndex >= 0 && startIndex < lines.size()) {
        mode = RefreshMode::None;
    }

    RVA offset = topOffset;
    if (mode == RefreshMode::None) {
        offset = lines[startIndex].offset;
    } else {
        if (count > 0) {
            offset = Core()->nextOpAddr(topOffset, count);
            if (offset < topOffset) {
                offset = RVA_MAX;
            }
        } else {
            offset = Core()->prevOpAddr(topOffset, -count);
            if (offset > topOffset) {
                offset = 0;
            }
        }
    }

    if (clampToScrollBarRange) {
        offset = mDisasScrollArea->verticalScrollBar()->clampAddressToRange(offset);
    }

    refreshDisasm(offset, mode);
    topOffsetHistory[topOffsetHistoryPos] = offset;
}

void DisassemblyWidget::updateSelection()
{
    int anchorRenderIndex = -1;
    int posRenderIndex = -1;

    auto findRenderIndex = [&](RVA offset, int offsetBlockIndex) -> int {
        // safely large number to clamp the highlight off-screen
        // without causing integer overflow during UI math
        constexpr int offscreenPadding = 100000;

        if (lines.isEmpty()) {
            return 0;
        }
        if (offset < lines.first().offset) {
            return -offscreenPadding;
        }
        if (offset > lines.last().offset) {
            return lines.size() + offscreenPadding;
        }

        const int exactIndex = getLineIndex(offset, offsetBlockIndex);
        if (exactIndex != -1) {
            return exactIndex;
        }

        // fallback: if somehow between bounds but missing, return nearest insertion point
        auto it = std::lower_bound(
                lines.begin(), lines.end(), offset,
                [](const DisassemblyLine &line, RVA target) { return line.offset < target; });
        return std::distance(lines.begin(), it);
    };

    anchorRenderIndex = findRenderIndex(selectionAnchorRVA, selectionAnchorSubIndex);
    posRenderIndex = findRenderIndex(selectionPosRVA, selectionPosSubIndex);

    // If the entire selection is completely above or completely below the screen then do nothing
    if ((anchorRenderIndex < startIndex && posRenderIndex < startIndex)
        || (anchorRenderIndex >= endIndex && posRenderIndex >= endIndex)) {
        mDisasTextEdit->textCursor().clearSelection();
        return;
    }

    // clamp indices so they don't reach outside current viewport
    const int clampedAnchorIndex = qBound(startIndex, anchorRenderIndex, endIndex - 1);
    const int clampedPosIndex = qBound(startIndex, posRenderIndex, endIndex - 1);

    const QTextBlock anchorBlock =
            mDisasTextEdit->document()->findBlockByNumber(clampedAnchorIndex - startIndex);
    const QTextBlock posBlock =
            mDisasTextEdit->document()->findBlockByNumber(clampedPosIndex - startIndex);

    if (anchorBlock.isValid() && posBlock.isValid()) {
        int finalAnchorPos = anchorBlock.position();
        int finalPosPos = posBlock.position();

        // if selection starts above visible lines/blocks then start the selection from first
        // character of first block otherwise start from where the selection actually started
        if (anchorRenderIndex >= startIndex) {
            finalAnchorPos += qMin(selectionAnchorChar, qMax(0, anchorBlock.length() - 1));
        }

        // same as above but for selection end
        if (posRenderIndex < endIndex) {
            finalPosPos += qMin(selectionPosChar, qMax(0, posBlock.length() - 1));
        } else {
            // The position is below the screen - select till end of the last visible block
            finalPosPos += qMax(0, posBlock.length() - 1);
        }

        QTextCursor restoreCursor(mDisasTextEdit->document());
        restoreCursor.setPosition(finalAnchorPos);
        restoreCursor.setPosition(finalPosPos, QTextCursor::KeepAnchor);
        mDisasTextEdit->setTextCursor(restoreCursor);
    }
}

void DisassemblyWidget::updateContextMenuSelection(bool hasSelection)
{
    mCtxMenu->setCanCopy(hasSelection);
    if (hasSelection) {
        mCtxMenu->setCurHighlightedWord(mDisasTextEdit->textCursor().selectedText());
    } else {
        mCtxMenu->setCurHighlightedWord(curHighlightedWord);
    }
}

int DisassemblyWidget::getLineIndex(RVA offset, int offsetSubIndex) const
{
    if (lines.isEmpty()) {
        return -1;
    }

    auto it = std::lower_bound(
            lines.begin(), lines.end(), offset,
            [](const DisassemblyLine &line, RVA target) { return line.offset < target; });

    if (it != lines.end() && it->offset == offset) {
        const int baseIndex = std::distance(lines.begin(), it);
        if (baseIndex + offsetSubIndex < lines.size()
            && lines[baseIndex + offsetSubIndex].offset == offset) {
            return baseIndex + offsetSubIndex;
        }
        return baseIndex;
    }
    return -1;
}

void DisassemblyWidget::updateLineHighlights()
{
    QList<QTextEdit::ExtraSelection> combinedSelections;
    combinedSelections.append(highlightCurrentLine());
    combinedSelections.append(highlightPCLine());
    mDisasTextEdit->setExtraSelections(combinedSelections);
}

void DisassemblyWidget::invalidateCursorSelection()
{
    selectionAnchorRVA = RVA_INVALID;
    selectionPosRVA = RVA_INVALID;
    selectionAnchorSubIndex = 0;
    selectionPosSubIndex = 0;
    selectionAnchorChar = 0;
    selectionPosChar = 0;
}

int DisassemblyWidget::getIndexInOffsetGroup(const QTextCursor &cursor) const
{
    const int lineIndex = startIndex + cursor.blockNumber();
    if (lineIndex < 0 || lineIndex >= lines.size()) {
        return 0;
    }

    const RVA offset = lines[lineIndex].offset;
    int subIndex = 0;

    for (int i = lineIndex - 1; i >= 0; --i) {
        if (lines[i].offset == offset) {
            subIndex++;
        } else {
            break; // offsets are contiguous
        }
    }
    return subIndex;
}

void DisassemblyWidget::updateSelectionPos(const QTextCursor &cursor)
{
    selectionPosRVA = DisHlp::readDisassemblyOffset(cursor);
    selectionPosSubIndex = getIndexInOffsetGroup(cursor);
    selectionPosChar = cursor.positionInBlock();
}

void DisassemblyWidget::updateSelectionAnchor(const QTextCursor &cursor)
{
    selectionAnchorRVA = DisHlp::readDisassemblyOffset(cursor);
    selectionAnchorSubIndex = getIndexInOffsetGroup(cursor);
    selectionAnchorChar = cursor.positionInBlock();
}

bool DisassemblyWidget::updateMaxLines()
{
    const int currentMaxLines = qhelpers::getMaxFullyDisplayedLines(mDisasTextEdit);

    if (currentMaxLines != maxLines) {
        maxLines = currentMaxLines;
        refreshDisasm();
        return true;
    }

    return false;
}

QList<QTextEdit::ExtraSelection> DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    const QColor highlightColor = ConfigColor("lineHighlight");

    const RVA currentOffset = seekable->getOffset();

    bool isOffScreen = true;
    if (currentOffset >= topOffset
        && (currentOffset <= bottomOffset || bottomOffset == RVA_INVALID)) {
        const int targetLineIndex = getLineIndex(currentOffset, cursorLineOffset);
        if (targetLineIndex >= startIndex && targetLineIndex < endIndex) {
            isOffScreen = false;
        }
    }

    QTextCursor cursor = mDisasTextEdit->textCursor();

    if (!isOffScreen) {
        if (cursor.hasSelection()) {
            curHighlightedWord = cursor.selectedText();
        } else {
            auto clickedCharPos = cursor.positionInBlock();
            // Select the line (BlockUnderCursor matches a line with current implementation)
            cursor.select(QTextCursor::BlockUnderCursor);
            // Remove any non-breakable space from the current line
            const QString searchString = cursor.selectedText().replace("\xc2\xa0", " ");
            // Cut the line in "tokens" that can be highlighted
            static const QRegularExpression tokenRegExp(R"(\b(?<!\.)([^\s]+)\b(?!\.))");
            QRegularExpressionMatchIterator i = tokenRegExp.globalMatch(searchString);
            curHighlightedWord.clear(); // Clear out old words if we click on empty space
            while (i.hasNext()) {
                const QRegularExpressionMatch match = i.next();
                // Current token is under our cursor, select this one
                if (match.capturedStart() <= clickedCharPos
                    && match.capturedEnd() > clickedCharPos) {
                    curHighlightedWord = match.captured();
                    break;
                }
            }
        }
    }

    // Highlight the current line
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = mDisasTextEdit->textCursor();
    highlightSelection.cursor.movePosition(QTextCursor::Start);
    while (true) {
        const RVA lineOffset = DisHlp::readDisassemblyOffset(highlightSelection.cursor);
        if (lineOffset == currentOffset) {
            highlightSelection.format.setBackground(highlightColor);
            highlightSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
            highlightSelection.cursor.clearSelection();
            extraSelections.append(highlightSelection);
        } else if (lineOffset != RVA_INVALID && lineOffset > currentOffset) {
            break;
        }
        highlightSelection.cursor.movePosition(QTextCursor::EndOfLine);
        if (highlightSelection.cursor.atEnd()) {
            break;
        }

        highlightSelection.cursor.movePosition(QTextCursor::Down);
    }

    // Highlight all the words in the document same as the current one
    if (!curHighlightedWord.isEmpty()) {
        extraSelections.append(createSameWordsSelections(mDisasTextEdit, curHighlightedWord));
    }

    return extraSelections;
}

QList<QTextEdit::ExtraSelection> DisassemblyWidget::highlightPCLine()
{
    const RVA pcAddr = Core()->getProgramCounterValue();

    const QColor highlightPCColor = ConfigColor("highlightPC");

    QList<QTextEdit::ExtraSelection> pcSelections;
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = mDisasTextEdit->textCursor();
    highlightSelection.cursor.movePosition(QTextCursor::Start);
    if (pcAddr != RVA_INVALID) {
        while (true) {
            const RVA lineOffset = DisHlp::readDisassemblyOffset(highlightSelection.cursor);
            if (lineOffset == pcAddr) {
                highlightSelection.format.setBackground(highlightPCColor);
                highlightSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
                highlightSelection.cursor.clearSelection();
                pcSelections.append(highlightSelection);
            } else if (lineOffset != RVA_INVALID && lineOffset > pcAddr) {
                break;
            }
            highlightSelection.cursor.movePosition(QTextCursor::EndOfLine);
            if (highlightSelection.cursor.atEnd()) {
                break;
            }

            highlightSelection.cursor.movePosition(QTextCursor::Down);
        }
    }

    return pcSelections;
}

void DisassemblyWidget::showDisasContextMenu(const QPoint &pt)
{
    mCtxMenu->exec(mDisasTextEdit->mapToGlobal(pt));
}

RVA DisassemblyWidget::readCurrentDisassemblyOffset()
{
    const QTextCursor tc = mDisasTextEdit->textCursor();
    return DisHlp::readDisassemblyOffset(tc);
}

void DisassemblyWidget::updateCursorPosition()
{
    const RVA offset = seekable->getOffset();

    connectCursorPositionChanged(true);

    const int targetLineIndex = getLineIndex(offset, cursorLineOffset);

    bool isOffScreen = false;
    if (offset < topOffset || (offset > bottomOffset && bottomOffset != RVA_INVALID)) {
        isOffScreen = true;
    } else if (targetLineIndex < startIndex || targetLineIndex >= endIndex) {
        isOffScreen = true;
    }

    if (isOffScreen) {
        mDisasTextEdit->moveCursor(QTextCursor::Start);
        mDisasTextEdit->setCursorVisible(false);
    } else {
        const int targetBlockNum = targetLineIndex - startIndex;
        QTextCursor cursor = mDisasTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        if (targetBlockNum > 0) {
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, targetBlockNum);
        }
        if (cursorCharOffset > 0) {
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, cursorCharOffset);
        }

        mDisasTextEdit->setTextCursor(cursor);
        mDisasTextEdit->setCursorVisible(true);
    }

    updateLineHighlights();

    connectCursorPositionChanged(false);
}

void DisassemblyWidget::connectCursorPositionChanged(bool disconnect)
{
    if (disconnect) {
        QObject::disconnect(mDisasTextEdit, &QPlainTextEdit::cursorPositionChanged, this,
                            &DisassemblyWidget::cursorPositionChanged);
    } else {
        connect(mDisasTextEdit, &QPlainTextEdit::cursorPositionChanged, this,
                &DisassemblyWidget::cursorPositionChanged);
    }
}

void DisassemblyWidget::cursorPositionChanged()
{
    const RVA offset = readCurrentDisassemblyOffset();

    const auto c = mDisasTextEdit->textCursor();
    cursorCharOffset = c.positionInBlock();
    cursorLineOffset = getIndexInOffsetGroup(c);

    seekFromCursor = true;
    seekable->seek(offset);
    seekFromCursor = false;

    updateLineHighlights();
    updateContextMenuSelection(mDisasTextEdit->textCursor().hasSelection());
    leftPanel->update();
}

void DisassemblyWidget::copySelection()
{
    if (selectionAnchorRVA == RVA_INVALID || selectionPosRVA == RVA_INVALID) {
        mDisasTextEdit->copy();
        return;
    }

    // determine direction (user could have dragged upwards)
    bool forward = true;
    if (selectionAnchorRVA > selectionPosRVA) {
        forward = false;
    } else if (selectionAnchorRVA == selectionPosRVA) {
        forward = (selectionAnchorChar <= selectionPosChar);
    }

    const RVA startRVA = forward ? selectionAnchorRVA : selectionPosRVA;
    const RVA endRVA = forward ? selectionPosRVA : selectionAnchorRVA;
    const int startChar = forward ? selectionAnchorChar : selectionPosChar;
    const int endChar = forward ? selectionPosChar : selectionAnchorChar;
    const int startRVAIndex = forward ? selectionAnchorSubIndex : selectionPosSubIndex;
    const int endRVAIndex = forward ? selectionPosSubIndex : selectionAnchorSubIndex;

    QStringList plainTextLines;
    QTextDocument htmlStripper;

    // check if the entire selection is currently sitting in 'lines' buffer
    int selectionStart = -1, selectionEnd = -1;
    if (!lines.isEmpty() && startRVA >= lines.first().offset && endRVA <= lines.last().offset) {
        selectionStart = getLineIndex(startRVA, startRVAIndex);
        selectionEnd = getLineIndex(endRVA, endRVAIndex);
    }

    // extract text
    if (selectionStart != -1 && selectionEnd != -1 && selectionStart <= selectionEnd) {
        for (int i = selectionStart; i <= selectionEnd; ++i) {
            htmlStripper.setHtml(lines[i].text);
            plainTextLines.append(htmlStripper.toPlainText());
        }
    } else {
        // selection exceeds buffer
        TempConfig tempConfig;
        tempConfig.set("scr.color", COLOR_MODE_DISABLED).set("asm.lines", false);

        RVA currentRVA = startRVA;
        int currentStartHit = 0;
        int currentEndHit = 0;

        while (currentRVA <= endRVA && currentRVA != RVA_INVALID) {
            const auto fetchedLines = Core()->disassembleLines(currentRVA, maxLines);
            if (fetchedLines.isEmpty()) {
                break;
            }

            for (const auto &line : fetchedLines) {
                // skip unselected duplicate lines at the top
                if (line.offset == startRVA && currentStartHit < startRVAIndex) {
                    currentStartHit++;
                    continue;
                }

                if (line.offset > endRVA) {
                    currentRVA = RVA_INVALID; // reached target
                    break;
                }

                plainTextLines.append(QString(line.text).replace("&nbsp;", " "));

                if (line.offset == endRVA) {
                    if (currentEndHit == endRVAIndex) {
                        currentRVA = RVA_INVALID; // reached target
                        break;
                    }
                    currentEndHit++;
                }
            }

            if (currentRVA != RVA_INVALID) {
                currentRVA = Core()->nextOpAddr(fetchedLines.last().offset, 1);
            }
        }
    }

    if (plainTextLines.isEmpty()) {
        return;
    }

    if (plainTextLines.size() == 1) {
        // single-line selection
        const int len = qMax(0, endChar - startChar);
        plainTextLines[0] = plainTextLines[0].mid(startChar, len);
    } else {
        // multi-line selection
        if (startChar > 0) {
            plainTextLines[0] = plainTextLines[0].mid(startChar);
        }
        if (endChar >= 0 && endChar < plainTextLines.last().length()) {
            plainTextLines.last() = plainTextLines.last().left(endChar);
        }
    }

    const QString finalCopiedText = plainTextLines.join("\n");
    QApplication::clipboard()->setText(finalCopiedText);
}

void DisassemblyWidget::moveCursorRelative(QTextCursor::MoveOperation op, bool page)
{
    const bool up = op == QTextCursor::Up;
    const bool select = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    const bool hadSelection = (selectionAnchorRVA != RVA_INVALID && selectionPosRVA != RVA_INVALID);

    if (select) {
        if (!hadSelection) {
            updateSelectionAnchor(mDisasTextEdit->textCursor());
        }
    } else {
        invalidateCursorSelection();
    }

    if (page) {
        RVA offset;
        if (!up) {
            offset = Core()->nextOpAddr(bottomOffset, 1);
            if (offset < bottomOffset) {
                offset = RVA_MAX;
            }
        } else {
            offset = Core()->prevOpAddr(topOffset, maxLines);
            if (offset > topOffset) {
                offset = 0;
            } else {
                // disassembly from calculated offset may have more than maxLines lines
                // move some instructions down if necessary.

                auto lines = Core()->disassembleLines(offset, maxLines).toVector();
                int oldTopLine;
                for (oldTopLine = lines.length(); oldTopLine > 0; oldTopLine--) {
                    if (lines[oldTopLine - 1].offset < topOffset) {
                        break;
                    }
                }

                int overflowLines = oldTopLine - maxLines;
                if (overflowLines > 0) {
                    while (lines[overflowLines - 1].offset == lines[overflowLines].offset
                           && overflowLines < lines.length() - 1) {
                        overflowLines++;
                    }
                    offset = lines[overflowLines].offset;
                }
            }
        }
        refreshDisasm(offset);

        if (select) {
            const auto newCursor = mDisasTextEdit->textCursor();
            selectionPosRVA = DisHlp::readDisassemblyOffset(newCursor);
            selectionPosSubIndex = getIndexInOffsetGroup(newCursor);
            selectionPosChar = newCursor.positionInBlock();
            refreshDisasm(RVA_INVALID, RefreshMode::None);
        } else if (hadSelection) {
            refreshDisasm(RVA_INVALID, RefreshMode::None);
        }
    } else {
        if (op == QTextCursor::Left || op == QTextCursor::Right) {
            mDisasTextEdit->moveCursor(op,
                                       select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);

            if (select) {
                updateSelectionPos(mDisasTextEdit->textCursor());
                refreshDisasm(RVA_INVALID, RefreshMode::None);
            } else if (hadSelection) {
                refreshDisasm(RVA_INVALID, RefreshMode::None);
            }
            return;
        }

        const int blockNumber = mDisasTextEdit->textCursor().blockNumber();
        const int count = endIndex - startIndex;

        if (up && blockNumber <= 0) {
            scrollInstructions(-1);
        } else if (op == QTextCursor::Down && blockNumber >= count - 1) {
            scrollInstructions(1);
        }

        mDisasTextEdit->moveCursor(op, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        mDisasTextEdit->verticalScrollBar()->setValue(startIndex - startIndex);

        if (select) {
            updateSelectionPos(mDisasTextEdit->textCursor());
            refreshDisasm(RVA_INVALID, RefreshMode::None);
        }

        const RVA offset = readCurrentDisassemblyOffset();
        if (offset != seekable->getOffset()) {
            seekable->seek(offset);
            updateLineHighlights();
        } else if (!select && hadSelection) {
            refreshDisasm(RVA_INVALID, RefreshMode::None);
        }
    }
}

void DisassemblyWidget::jumpToOffsetUnderCursor(const QTextCursor &cursor)
{
    const RVA offset = DisHlp::readDisassemblyOffset(cursor);
    seekable->seekToReference(offset);
}

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (obj == mDisasTextEdit || obj == mDisasTextEdit->viewport())) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            auto ctx = DisHlp::getContextFromCursor(
                    mDisasTextEdit->cursorForPosition(mouseEvent->pos()));

            const DisHlp::TargetAction ta =
                    DisHlp::resolveTarget(ctx, DisassemblyHelper::TargetFilter::Standard);
            switch (ta.type) {
            case DisHlp::TargetType::TypeName:
                Core()->showTypeInTypesWidget(ctx.word);
                break;
            case DisHlp::TargetType::XRefComment:
            case DisHlp::TargetType::VariableXRef:
            case DisHlp::TargetType::Arrow:
                if (ta.value != RVA_INVALID) {
                    seekable->seek(ta.value);
                }
                break;
            case DisHlp::TargetType::None:
                seekable->seekToReference(ctx.offset);
                break;
            default:
                break;
            }
            return true;
        }
    } else if ((Config()->getPreviewValue() || Config()->getShowVarTooltips())
               && event->type() == QEvent::ToolTip && obj == mDisasTextEdit->viewport()) {
        const auto *helpEvent = static_cast<QHelpEvent *>(event);

        auto ctx =
                DisHlp::getContextFromCursor(mDisasTextEdit->cursorForPosition(helpEvent->pos()));

        return DisassemblyPreview::showTooltip(this, helpEvent->globalPos(), ctx,
                                               Config()->getPreviewValue());
    }

    return MemoryDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return) {
        const QTextCursor cursor = mDisasTextEdit->textCursor();
        auto ta = DisHlp::resolveTarget(DisHlp::getContextFromCursor(cursor),
                                        DisHlp::TargetFilter::Arrows);
        if (ta.type == DisHlp::TargetType::Arrow) {
            seekable->seek(ta.value);
        } else {
            jumpToOffsetUnderCursor(cursor);
        }
        event->accept();
        return;
    }

    // handle cursor shortcuts here because we need to check if shift is pressed for
    // updating selection.
    // shift select is not supported for page up and page down currently
    bool handled = false;
    QTextCursor::MoveOperation op;

    // fix for macos, since macos appends KeypadModifier to cursor keys pressed via keypad, it
    // causes the QKeySequence::ExactMatch check to fail.
    // We remove the modifier here
    const Qt::KeyboardModifiers pureModifiers = event->modifiers() & ~Qt::KeypadModifier;

    const int baseKey = event->key() | (pureModifiers & ~Qt::ShiftModifier);
    QKeySequence baseSeq(baseKey);
    QKeySequence exactSeq(event->key() | event->modifiers());

    auto matches = [&](const QString &id) {
        for (const QKeySequence &seq : Shortcuts()->getKeySequences(id)) {
            if (seq.matches(baseSeq) == QKeySequence::ExactMatch
                || seq.matches(exactSeq) == QKeySequence::ExactMatch) {
                return true;
            }
        }
        return false;
    };

    if (matches("Disassembly.moveUp")) {
        op = QTextCursor::Up;
        handled = true;
    } else if (matches("Disassembly.moveDown")) {
        op = QTextCursor::Down;
        handled = true;
    } else if (matches("Disassembly.moveLeft")) {
        op = QTextCursor::Left;
        handled = true;
    } else if (matches("Disassembly.moveRight")) {
        op = QTextCursor::Right;
        handled = true;
    }

    if (handled) {
        moveCursorRelative(op, false);
        mDisasTextEdit->setCursorVisible(true);
        event->accept();
        return;
    }

    MemoryDockWidget::keyPressEvent(event);
}

void DisassemblyWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (event->reason() == QContextMenuEvent::Keyboard) {
        showDisasContextMenu(event->pos());
        event->accept();
        return;
    }

    MemoryDockWidget::contextMenuEvent(event);
}

QString DisassemblyWidget::getWindowTitle() const
{
    return tr("Disassembly");
}

int DisassemblyWidget::getEndIndex() const
{
    return endIndex;
}

int DisassemblyWidget::getStartIndex() const
{
    return startIndex;
}

void DisassemblyWidget::onSeekChanged(RVA offset, CutterCore::SeekHistoryType type)
{
    if (type == CutterCore::SeekHistoryType::New) {
        // Erase previous history past this point.
        if (topOffsetHistory.size() > topOffsetHistoryPos + 1) {
            topOffsetHistory.erase(topOffsetHistory.begin() + topOffsetHistoryPos + 1,
                                   topOffsetHistory.end());
        }
        topOffsetHistory.push_back(offset);
        topOffsetHistoryPos = topOffsetHistory.size() - 1;
    } else if (type == CutterCore::SeekHistoryType::Undo) {
        --topOffsetHistoryPos;
    } else if (type == CutterCore::SeekHistoryType::Redo) {
        ++topOffsetHistoryPos;
    }
    if (!seekFromCursor) {
        cursorLineOffset = 0;
        cursorCharOffset = 0;

        // invalidate selection if the seek came from an external
        // source (like seeking via VisualNavBar)
        invalidateCursorSelection();
    }

    if (topOffset != RVA_INVALID && offset >= topOffset && offset <= bottomOffset
        && type == CutterCore::SeekHistoryType::New) {
        // if the line with the seek offset is currently visible, just move the cursor there
        updateCursorPosition();
        topOffsetHistory[topOffsetHistoryPos] = topOffset;
    } else {
        // otherwise scroll there
        refreshDisasm(topOffsetHistory[topOffsetHistoryPos]);
    }
    mCtxMenu->setOffset(offset);
    // after seek it will select curret instruction and updates renaming options
    mCtxMenu->setCurHighlightedWord(curHighlightedWord);
}

void DisassemblyWidget::fontsUpdatedSlot()
{
    setupFonts();

    if (!updateMaxLines()) { // updateMaxLines() returns true if it already refreshed.
        refreshDisasm();
    }
}

void DisassemblyWidget::colorsUpdatedSlot()
{
    setupColors();
    refreshDisasm();
}

void DisassemblyWidget::setupFonts()
{
    mDisasTextEdit->setFont(Config()->getFont());
}

void DisassemblyWidget::setupColors()
{
    mDisasTextEdit->setStyleSheet(QString("QPlainTextEdit { background-color: %1; color: %2; }")
                                          .arg(ConfigColor("gui.background").name())
                                          .arg(ConfigColor("btext").name()));

    // Read and set a stylesheet for the QToolTip too
    setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
}

DisassemblyScrollArea::DisassemblyScrollArea(QWidget *parent)
    : QAbstractScrollArea(parent),
      vScrollBar(new AddressRangeScrollBar(this)),
      accumScrollWheelDeltaY(0)
{

    setVerticalScrollBar(vScrollBar);

    vScrollBar->setPageStep(40);
    vScrollBar->setSingleStep(1);
    connect(vScrollBar, &AddressRangeScrollBar::scrolled, this,
            [this](int lines) { emit scrollLines(-lines, true); });
    connect(vScrollBar, &AddressRangeScrollBar::hideScrollBar, this,
            [this]() { setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); });
    connect(vScrollBar, &AddressRangeScrollBar::showScrollBar, this,
            [this]() { setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); });
    vScrollBar->refreshRange();
}

AddressRangeScrollBar *DisassemblyScrollArea::verticalScrollBar()
{
    return vScrollBar;
}

bool DisassemblyScrollArea::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        emit disassemblyResized();
    }

    return QAbstractScrollArea::viewportEvent(event);
}

void DisassemblyScrollArea::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().isNull() || !event->angleDelta().y()) {
        QAbstractScrollArea::wheelEvent(event);
        return;
    }

    accumScrollWheelDeltaY += event->angleDelta().y();
    // Delta is reported in 1/8 of a degree
    // eg. 120 units * 1/8 = 15 degrees
    // Typical scroll speed is 1 line per 5 degrees
    const int lineDelta = 5 * 8;
    if (accumScrollWheelDeltaY >= lineDelta || accumScrollWheelDeltaY <= -lineDelta) {
        const int lineCount = accumScrollWheelDeltaY / lineDelta;
        accumScrollWheelDeltaY -= lineDelta * lineCount;
        emit scrollLines(-lineCount);
    }
    emit wheelEventTriggered();
}

DisassemblyTextEdit::DisassemblyTextEdit(DisassemblyWidget *disasmWidget)
    : QPlainTextEdit(disasmWidget), lockScroll(false), disasmWidget(disasmWidget)
{

    blinkTimer = new QTimer(this);
    connect(blinkTimer, &QTimer::timeout, this, [this]() {
        cursorVisible = !cursorVisible;
        viewport()->update();
    });
    blinkTimer->start(500);
}

qreal DisassemblyTextEdit::textOffset() const
{
    return (blockBoundingGeometry(document()->begin()).topLeft() + contentOffset()).y();
}

void DisassemblyTextEdit::setCursorVisible(bool visible)
{
    if (visible) {
        cursorVisible = true;
        blinkTimer->start(500);
    } else {
        cursorVisible = false;
        blinkTimer->stop();
    }
    viewport()->update();
}

bool DisassemblyTextEdit::viewportEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Type::Wheel:
        return false;
    default:
        return QAbstractScrollArea::viewportEvent(event);
    }
}

void DisassemblyTextEdit::scrollContentsBy(int dx, int dy)
{
    if (!lockScroll) {
        QPlainTextEdit::scrollContentsBy(dx, dy);
    }
}

void DisassemblyTextEdit::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
    // QPlainTextEdit::keyPressEvent(event);
}

void DisassemblyTextEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {

        const QTextCursor cursor = cursorForPosition(event->pos());

        disasmWidget->updateSelectionAnchor(cursor);
        disasmWidget->updateSelectionPos(cursor);

        disasmWidget->refreshDisasm(RVA_INVALID, RefreshMode::None);
    }

    QPlainTextEdit::mousePressEvent(event);

    if (event->button() == Qt::RightButton && !textCursor().hasSelection()) {
        setTextCursor(cursorForPosition(event->pos()));
    }
}

void DisassemblyTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {

        const QPoint pos = event->pos();
        const QTextCursor cursor = cursorForPosition(pos);
        const RVA currentRVA = DisHlp::readDisassemblyOffset(cursor);

        if (currentRVA != RVA_INVALID) {
            disasmWidget->updateSelectionPos(cursor);
        }

        if (pos.y() < 0) {
            disasmWidget->scrollInstructions(-1, true);
        } else if (pos.y() > viewport()->height()) {
            disasmWidget->scrollInstructions(1, true);
        } else if (currentRVA != RVA_INVALID) {
            disasmWidget->refreshDisasm(RVA_INVALID, RefreshMode::None);
        }

        return;
    }

    QPlainTextEdit::mouseMoveEvent(event);
}

void DisassemblyTextEdit::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    if (cursorVisible) {
        QPainter painter(viewport());
        QRect cRect = cursorRect();
        cRect.setWidth(2);
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.fillRect(cRect, Qt::white);
    }
}

void DisassemblyWidget::seekPrev()
{
    Core()->seekPrev();
}

/*********************
 * Left panel
 *********************/

DisassemblyLeftPanel::DisassemblyLeftPanel(DisassemblyWidget *disas) : disas(disas)
{

    arrows.reserve((arrowsSize * 3) / 2);
}

void DisassemblyLeftPanel::wheelEvent(QWheelEvent *event)
{

    int count = -(event->angleDelta() / 15).y();
    count -= (count > 0 ? 5 : -5);

    this->disas->showTransientScrollBar();
    this->disas->scrollInstructions(count);
}

void DisassemblyLeftPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    constexpr int penSizePix = 1;
    constexpr int distanceBetweenLines = 10;
    constexpr int arrowWidth = 5;
    const int rightOffset = size().rwidth();
    auto tEdit = qobject_cast<DisassemblyTextEdit *>(disas->getTextWidget());
    int lineHeight = qCeil(disas->getFontMetrics().lineSpacing());
    const QColor arrowColorDown = ConfigColor("flow");
    const QColor arrowColorUp = ConfigColor("cflow");
    QPainter p(this);
    const QPen penDown(arrowColorDown, penSizePix, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
    const QPen penUp(arrowColorUp, penSizePix, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
    // Fill background
    p.fillRect(event->rect(), Config()->getColor("gui.background").darker(115));

    QList<DisassemblyLine> lines = disas->getLines();
    if (lines.size() == 0) {
        // No line to print, abort early
        return;
    }

    std::vector<int> lineY(tEdit->document()->lineCount());
    QTextCursor cursor(tEdit->document());
    for (auto &line : lineY) {
        auto rect = tEdit->cursorRect(cursor);
        line = rect.top();
        cursor.movePosition(QTextCursor::Down);
    }

    const int startIndex = disas->getStartIndex();
    const int endIndex = qMin(static_cast<int>(lines.size()), disas->getEndIndex());
    const int maxLines = endIndex - startIndex;

    // Capture how many lines are ACTUALLY drawn on the screen right now
    const int visibleCount = qMin(maxLines, static_cast<int>(lines.size()) - startIndex);
    if (visibleCount <= 0) {
        return;
    }

    using LineInfo = std::pair<RVA, int>;
    std::vector<LineInfo> lineOffsets;
    lineOffsets.reserve(visibleCount + arrows.size());

    RVA minViewOffset = lines[startIndex].offset;
    RVA maxViewOffset = minViewOffset;

    for (int i = startIndex; i < endIndex; i++) {
        // Map offset to a viewport-relative index (0 to maxLines) instead of absolute
        // buffer index 'i'
        lineOffsets.emplace_back(lines[i].offset, i - startIndex);
        minViewOffset = std::min(minViewOffset, lines[i].offset);
        maxViewOffset = std::max(maxViewOffset, lines[i].offset);
        if (lines[i].arrow != RVA_INVALID) {
            Arrow a { lines[i].offset, lines[i].arrow };
            const bool contains = std::find_if(std::begin(arrows), std::end(arrows),
                                               [&](const Arrow &it) {
                                                   return it.min == a.min && it.max == a.max;
                                               })
                    != std::end(arrows);
            if (!contains) {
                arrows.emplace_back(lines[i].offset, lines[i].arrow);
            }
        }
    }

    auto addOffsetOutsideScreen = [&](RVA offset) {
        if (offset < minViewOffset || offset > maxViewOffset) {
            lineOffsets.emplace_back(offset, -1);
        }
    };

    // Assign sequential numbers to offsets outside screen while preserving their relative order.
    // Preserving relative order helps reducing reordering while scrolling. Using sequential numbers
    // allows using data structures designed for dense ranges.
    for (auto &arrow : arrows) {
        addOffsetOutsideScreen(arrow.min);
        addOffsetOutsideScreen(arrow.max);
    }
    std::sort(lineOffsets.begin(), lineOffsets.end());
    lineOffsets.erase(std::unique(lineOffsets.begin(), lineOffsets.end()), lineOffsets.end());
    const size_t firstVisibleLine =
            std::find_if(lineOffsets.begin(), lineOffsets.end(),
                         [](const LineInfo &line) { return line.second == 0; })
            - lineOffsets.begin();
    for (int i = int(firstVisibleLine) - 1; i >= 0; i--) {
        // -1 to ensure end of arrrow is drawn outside screen
        lineOffsets[i].second = i - firstVisibleLine - 1;
    }
    const size_t firstLineAfter =
            std::find_if(lineOffsets.begin(), lineOffsets.end(),
                         [&](const LineInfo &line) { return line.first > maxViewOffset; })
            - lineOffsets.begin();
    for (size_t i = firstLineAfter; i < lineOffsets.size(); i++) {
        lineOffsets[i].second = visibleCount + (i - firstLineAfter)
                + 1; // +1 to ensure end of arrrow is drawn outside screen
    }

    auto offsetToLine = [&](RVA offset) -> int {
        // binary search because linesPixPosition is sorted by offset
        if (lineOffsets.empty()) {
            return 0;
        }
        if (offset < lineOffsets[0].first) {
            return lineOffsets[0].second - 1;
        }
        auto res = lower_bound(std::begin(lineOffsets), std::end(lineOffsets), offset,
                               [](const LineInfo &it, RVA offset) { return it.first < offset; });
        if (res == std::end(lineOffsets)) {
            return lineOffsets.back().second + 1;
        }
        return res->second;
    };

    std::sort(std::begin(arrows), std::end(arrows), [](const Arrow &l, const Arrow &r) {
        const auto lLen = l.length();
        const auto rLen = r.length();
        if (lLen != rLen) {
            return lLen < rLen;
        }
        return l.max != r.max ? l.max < r.max : l.min > r.min;
    });

    int minLine = 0, maxLine = 0;
    for (auto &it : arrows) {
        minLine = std::min(offsetToLine(it.min), minLine);
        maxLine = std::max(offsetToLine(it.max), maxLine);
        it.level = 0;
    }

    const int maxArrowLines = 1 << 18;
    uint32_t maxLevel = 0;
    if (!arrows.empty() && maxLine - minLine < maxArrowLines) {
        // Limit maximum tree range to MAX_ARROW_LINES as sanity check, since the tree is designed
        // for dense ranges. Under normal conditions due to amount lines fitting screen and number
        // of arrows remembered should be few hundreds at most.
        MinMaxAccumulateTree<uint32_t> maxLevelTree(maxLine - minLine + 2);
        for (Arrow &arrow : arrows) {
            const int top = offsetToLine(arrow.min) - minLine;
            const int bottom = offsetToLine(arrow.max) - minLine + 1;
            auto minMax = maxLevelTree.rangeMinMax(top, bottom);
            if (minMax.first > 1) {
                arrow.level = 1; // place below existing lines
            } else {
                arrow.level = minMax.second + 1; // place on top of existing lines
                maxLevel = std::max(maxLevel, arrow.level);
            }
            maxLevelTree.updateRange(top, bottom, arrow.level);
        }
    }

    const RVA currOffset = disas->getSeekable()->getOffset();
    const qreal pixelRatio = qhelpers::devicePixelRatio(p.device());

    const int targetEndIndex =
            qMin(static_cast<int>(lines.size() - 1), startIndex + visibleCount - 1);
    const Arrow visibleRange { lines.at(startIndex).offset, lines.at(targetEndIndex).offset };

    // Draw the lines
    for (const auto &arrow : arrows) {
        if (!visibleRange.intersects(arrow)) {
            continue;
        }
        const int lineOffset =
                int((distanceBetweenLines * arrow.level + distanceBetweenLines) * pixelRatio);

        p.setPen(arrow.up ? penUp : penDown);
        if (arrow.min == currOffset || arrow.max == currOffset) {
            QPen pen = p.pen();
            pen.setWidthF((penSizePix * 3) / 2.0);
            p.setPen(pen);
        }

        auto lineToPixels = [&](int i) {
            const int offset = int(arrow.up ? std::floor(pixelRatio) : -std::floor(pixelRatio));
            const int clampedLine = std::max(0, std::min(i, ((int)lineY.size()) - 1));
            int pos0 = 0;
            if (lineY.size() > 0) {
                pos0 = lineY[clampedLine];
            }
            return pos0 + (i - clampedLine) * lineHeight + lineHeight / 2 + offset;
        };

        const int lineStartNumber = offsetToLine(arrow.jmpFromOffset());
        int currentLineYPos = lineToPixels(lineStartNumber);

        const int arrowLineNumber = offsetToLine(arrow.jmpToffset());
        int lineArrowY = lineToPixels(arrowLineNumber);

        if (lineStartNumber == arrowLineNumber) {
            currentLineYPos += lineHeight / 4;
            lineArrowY -= lineHeight / 4;
        }

        // Draw the lines
        p.drawLine(rightOffset, currentLineYPos, rightOffset - lineOffset, currentLineYPos); // left
        p.drawLine(rightOffset - lineOffset, currentLineYPos, rightOffset - lineOffset,
                   lineArrowY); // vertical

        p.drawLine(rightOffset - lineOffset, lineArrowY, rightOffset, lineArrowY); // right

        { // triangle
            QPainterPath arrow;
            arrow.moveTo(rightOffset - arrowWidth, lineArrowY + arrowWidth);
            arrow.lineTo(rightOffset - arrowWidth, lineArrowY - arrowWidth);
            arrow.lineTo(rightOffset, lineArrowY);
            p.fillPath(arrow, p.pen().brush());
        }
    }

    if (maxLevel > maxLevelBeforeFlush) {
        arrows.clear();
    }

    const size_t eraseN = arrows.size() > arrowsSize ? arrows.size() - arrowsSize : 0;
    if (eraseN > 0) {
        const bool scrolledDown = lastBeginOffset > lines.first().offset;
        std::sort(std::begin(arrows), std::end(arrows), [&](const Arrow &l, const Arrow &r) {
            if (scrolledDown) {
                return l.jmpFromOffset() < r.jmpFromOffset();
            } else {
                return l.jmpFromOffset() > r.jmpFromOffset();
            }
        });
        arrows.erase(std::end(arrows) - eraseN, std::end(arrows));
    }

    lastBeginOffset = lines.first().offset;
}

void DisassemblyLeftPanel::clearArrowFrom(RVA offset)
{
    auto it = std::find_if(arrows.begin(), arrows.end(),
                           [&](const Arrow &it) { return it.jmpFromOffset() == offset; });
    if (it != arrows.end()) {
        arrows.erase(it);
    }
}
