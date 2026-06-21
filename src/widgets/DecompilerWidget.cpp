#include "DecompilerWidget.h"

#include "common/Configuration.h"
#include "common/CutterSeekable.h"
#include "common/Decompiler.h"
#include "common/DecompilerHighlighter.h"
#include "common/Helpers.h"
#include "common/SelectionHighlight.h"
#include "core/MainWindow.h"
#include "menus/DecompilerContextMenu.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_DecompilerWidget.h"

#include <QAbstractSlider>
#include <QClipboard>
#include <QObject>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBlockUserData>
#include <QTextEdit>

DecompilerWidget::DecompilerWidget(MainWindow *main)
    : MemoryDockWidget(MemoryWidgetType::Decompiler, main),
      mCtxMenu(new DecompilerContextMenu(this, main)),
      ui(new Ui::DecompilerWidget),
      refreshDeferrer(createRefreshDeferrer([this]() { doRefresh(); })),
      decompilerBusy(false),
      seekFromCursor(false),
      historyPos(0),
      previousFunctionAddr(RVA_INVALID),
      decompiledFunctionAddr(RVA_INVALID),
      code(Decompiler::makeWarning(tr("Choose an offset and refresh to get decompiled code")),
           &rz_annotated_code_free)
{
    ui->setupUi(this);
    setObjectName(main ? main->getUniqueObjectName(getWidgetType()) : getWidgetType());
    updateWindowTitle();

    setHighlighter(Config()->isDecompilerAnnotationHighlighterEnabled());
    // Event filter to intercept double click and right click in the textbox
    ui->textEdit->viewport()->installEventFilter(this);

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), &Configuration::fontsUpdated, this, &DecompilerWidget::fontsUpdatedSlot);
    connect(Config(), &Configuration::colorsUpdated, this, &DecompilerWidget::colorsUpdatedSlot);
    connect(Core(), &CutterCore::registersChanged, this, &DecompilerWidget::highlightPC);
    connect(mCtxMenu, &DecompilerContextMenu::copy, this, &DecompilerWidget::copy);

    auto decompilers = Core()->getDecompilers();
    QString selectedDecompilerId = Config()->getSelectedDecompiler();
    if (selectedDecompilerId.isEmpty()) {
        // If no decompiler was previously chosen. set rz-ghidra as default decompiler
        selectedDecompilerId = "ghidra";
    }
    for (const Decompiler *dec : std::as_const(decompilers)) {
        ui->decompilerComboBox->addItem(dec->getName(), dec->getId());
        if (dec->getId() == selectedDecompilerId) {
            ui->decompilerComboBox->setCurrentIndex(ui->decompilerComboBox->count() - 1);
        }
    }
    decompilerSelectionEnabled = decompilers.size() > 1;
    ui->decompilerComboBox->setEnabled(decompilerSelectionEnabled);
    if (decompilers.isEmpty()) {
        ui->textEdit->setPlainText(tr("No Decompiler available."));
    }

    connect(ui->decompilerComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &DecompilerWidget::decompilerSelected);
    connectCursorPositionChanged(true);
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &DecompilerWidget::seekChanged);
    ui->textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit, &QWidget::customContextMenuRequested, this,
            &DecompilerWidget::showDecompilerContextMenu);

    connect(Core(), &CutterCore::breakpointsChanged, this, &DecompilerWidget::updateBreakpoints);
    mCtxMenu->addSeparator();
    mCtxMenu->addAction(&syncAction);
    addActions(mCtxMenu->actions());

    ui->progressLabel->setVisible(false);
    doRefresh();

    connect(Core(), &CutterCore::refreshAll, this, &DecompilerWidget::doRefresh);
    connect(Core(), &CutterCore::functionRenamed, this, &DecompilerWidget::doRefresh);
    connect(Core(), &CutterCore::varsChanged, this, &DecompilerWidget::doRefresh);
    connect(Core(), &CutterCore::functionsChanged, this, &DecompilerWidget::doRefresh);
    connect(Core(), &CutterCore::flagsChanged, this, &DecompilerWidget::doRefresh);
    connect(Core(), &CutterCore::globalVarsChanged, this, &DecompilerWidget::doRefresh);
    connect(Core(), &CutterCore::commentsChanged, this, &DecompilerWidget::refreshIfChanged);
    connect(Core(), &CutterCore::instructionChanged, this, &DecompilerWidget::refreshIfChanged);
    connect(Core(), &CutterCore::refreshCodeViews, this, &DecompilerWidget::doRefresh);

    // Esc to seek backward
    QAction *seekPrevAction = Shortcuts()->makeAction("General.seekPrev", this);
    seekPrevAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(seekPrevAction);
    connect(seekPrevAction, &QAction::triggered, seekable, &CutterSeekable::seekPrev);
}

DecompilerWidget::~DecompilerWidget() = default;

QString DecompilerWidget::getWidgetType()
{
    return "DecompilerWidget";
}

Decompiler *DecompilerWidget::getCurrentDecompiler()
{
    return Core()->getDecompilerById(ui->decompilerComboBox->currentData().toString());
}

ut64 DecompilerWidget::findReference(size_t pos)
{
    size_t closestPos = SIZE_MAX;
    ut64 closestOffset = RVA_INVALID;
    RzCodeAnnotation *annotation = nullptr;
    CutterRzVectorForeach(&code->annotations, annotation, RzCodeAnnotation)
    {
        if (!(annotation->type == RZ_CODE_ANNOTATION_TYPE_GLOBAL_VARIABLE)
            || annotation->start > pos || annotation->end <= pos) {
            continue;
        }
        if (closestPos != SIZE_MAX && closestPos >= annotation->start) {
            continue;
        }
        closestPos = annotation->start;
        closestOffset = annotation->reference.offset;
    }
    return closestOffset;
}

ut64 DecompilerWidget::offsetForPosition(size_t pos)
{
    size_t closestPos = SIZE_MAX;
    ut64 closestOffset = mCtxMenu->getFirstOffsetInLine();
    RzCodeAnnotation *annotation;
    CutterRzVectorForeach(&code->annotations, annotation, RzCodeAnnotation)
    {
        if (!(annotation->type == RZ_CODE_ANNOTATION_TYPE_OFFSET) || annotation->start > pos
            || annotation->end <= pos) {
            continue;
        }
        if (closestPos != SIZE_MAX && closestPos >= annotation->start) {
            continue;
        }
        closestPos = annotation->start;
        closestOffset = annotation->offset.offset;
    }
    return closestOffset;
}

size_t DecompilerWidget::positionForOffset(ut64 offset)
{
    size_t closestPos = SIZE_MAX;
    ut64 closestOffset = UT64_MAX;
    RzCodeAnnotation *annotation = nullptr;

    CutterRzVectorForeach(&code->annotations, annotation, RzCodeAnnotation)
    {
        if (annotation->type != RZ_CODE_ANNOTATION_TYPE_OFFSET
            || annotation->offset.offset > offset) {
            continue;
        }
        if (closestOffset != UT64_MAX && closestOffset >= annotation->offset.offset) {
            continue;
        }
        closestPos = annotation->start;
        closestOffset = annotation->offset.offset;
    }
    return closestPos;
}

void DecompilerWidget::updateBreakpoints(RVA addr)
{
    if (!addressInRange(addr)) {
        return;
    }
    setInfoForBreakpoints();
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(QTextCharFormat());
    cursor.setBlockFormat(QTextBlockFormat());
    ui->textEdit->setExtraSelections({});
    highlightPC();
    highlightBreakpoints();
    updateSelection();
}

void DecompilerWidget::setInfoForBreakpoints()
{
    if (mCtxMenu->getIsTogglingBreakpoints()) {
        return;
    }
    // Get the range of the line
    QTextCursor cursorForLine = ui->textEdit->textCursor();
    cursorForLine.movePosition(QTextCursor::StartOfLine);
    const size_t startPos = cursorForLine.position();
    cursorForLine.movePosition(QTextCursor::EndOfLine);
    const size_t endPos = cursorForLine.position();
    gatherBreakpointInfo(*code, startPos, endPos);
}

void DecompilerWidget::gatherBreakpointInfo(RzAnnotatedCode &codeDecompiled, size_t startPos,
                                            size_t endPos)
{
    RVA firstOffset = RVA_MAX;
    RzCodeAnnotation *annotation;
    CutterRzVectorForeach(&codeDecompiled.annotations, annotation, RzCodeAnnotation)
    {
        if (annotation->type != RZ_CODE_ANNOTATION_TYPE_OFFSET) {
            continue;
        }
        if ((startPos <= annotation->start && annotation->start < endPos)
            || (startPos < annotation->end && annotation->end < endPos)) {
            firstOffset = (annotation->offset.offset < firstOffset) ? annotation->offset.offset
                                                                    : firstOffset;
        }
    }
    mCtxMenu->setFirstOffsetInLine(firstOffset);
    const QList<RVA> functionBreakpoints = Core()->getBreakpointsInFunction(decompiledFunctionAddr);
    QVector<RVA> offsetList;
    for (const RVA bpOffset : functionBreakpoints) {
        const size_t pos = positionForOffset(bpOffset);
        if (startPos <= pos && pos <= endPos) {
            offsetList.push_back(bpOffset);
        }
    }
    std::sort(offsetList.begin(), offsetList.end());
    mCtxMenu->setAvailableBreakpoints(offsetList);
}

void DecompilerWidget::refreshIfChanged(RVA addr)
{
    if (addressInRange(addr)) {
        doRefresh();
    }
}

void DecompilerWidget::doRefresh()
{
    const RVA addr = seekable->getOffset();
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }
    if (ui->decompilerComboBox->currentIndex() < 0) {
        return;
    }
    Decompiler *dec = getCurrentDecompiler();
    if (!dec) {
        return;
    }
    // Disabling decompiler selection combo box and making progress label visible ahead of
    // decompilation.
    ui->progressLabel->setVisible(true);
    ui->decompilerComboBox->setEnabled(false);
    if (dec->isRunning()) {
        if (!decompilerBusy) {
            connect(dec, &Decompiler::finished, this, &DecompilerWidget::doRefresh);
        }
        return;
    }
    disconnect(dec, &Decompiler::finished, this, &DecompilerWidget::doRefresh);
    // Clear all selections since we just refreshed
    ui->textEdit->setExtraSelections({});
    previousFunctionAddr = decompiledFunctionAddr;
    decompiledFunctionAddr = Core()->getFunctionStart(addr);
    updateWindowTitle();
    if (decompiledFunctionAddr == RVA_INVALID) {
        // No function was found, so making the progress label invisible and enabling
        // the decompiler selection combo box as we are not waiting for any decompilation to finish.
        ui->progressLabel->setVisible(false);
        ui->decompilerComboBox->setEnabled(true);
        setCode(Decompiler::makeWarning(
                tr("No function found at this offset. "
                   "Seek to a function or define one in order to decompile it.")));
        return;
    }
    mCtxMenu->setDecompiledFunctionAddress(decompiledFunctionAddr);
    connect(dec, &Decompiler::finished, this, &DecompilerWidget::decompilationFinished);
    decompilerBusy = true;
    dec->decompileAt(addr);
}

void DecompilerWidget::refreshDecompiler()
{
    doRefresh();
    setInfoForBreakpoints();
}

QTextCursor DecompilerWidget::getCursorForAddress(RVA addr)
{
    const size_t pos = positionForOffset(addr);
    if (pos == SIZE_MAX || pos == 0) {
        return QTextCursor();
    }
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(pos);
    return cursor;
}

void DecompilerWidget::decompilationFinished(RzAnnotatedCode *codeDecompiled)
{
    ui->progressLabel->setVisible(false);
    ui->decompilerComboBox->setEnabled(decompilerSelectionEnabled);

    mCtxMenu->setAnnotationHere(nullptr);
    setCode(codeDecompiled);

    const Decompiler *dec = getCurrentDecompiler();
    QObject::disconnect(dec, &Decompiler::finished, this, &DecompilerWidget::decompilationFinished);
    decompilerBusy = false;

    if (ui->textEdit->toPlainText().isEmpty()) {
        setCode(Decompiler::makeWarning(tr("Cannot decompile at this address (Not a function?)")));
        lowestOffsetInCode = RVA_MAX;
        highestOffsetInCode = 0;
        return;
    } else {
        updateCursorPosition();
        highlightPC();
        highlightBreakpoints();
        lowestOffsetInCode = RVA_MAX;
        highestOffsetInCode = 0;
        RzCodeAnnotation *annotation;
        CutterRzVectorForeach(&code->annotations, annotation, RzCodeAnnotation)
        {
            if (annotation->type == RZ_CODE_ANNOTATION_TYPE_OFFSET) {
                if (lowestOffsetInCode > annotation->offset.offset) {
                    lowestOffsetInCode = annotation->offset.offset;
                }
                if (highestOffsetInCode < annotation->offset.offset) {
                    highestOffsetInCode = annotation->offset.offset;
                }
            }
        }
    }

    if (!scrollHistory.empty()) {
        ui->textEdit->horizontalScrollBar()->setSliderPosition(scrollHistory[historyPos].first);
        ui->textEdit->verticalScrollBar()->setSliderPosition(scrollHistory[historyPos].second);
    }
}

void DecompilerWidget::setAnnotationsAtCursor(size_t pos)
{
    RzCodeAnnotation *annotationAtPos = nullptr;
    RzCodeAnnotation *annotation;
    CutterRzVectorForeach(&this->code->annotations, annotation, RzCodeAnnotation)
    {
        if (annotation->type == RZ_CODE_ANNOTATION_TYPE_OFFSET
            || annotation->type == RZ_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT
            || annotation->start > pos || annotation->end <= pos) {
            continue;
        }
        annotationAtPos = annotation;
        break;
    }
    mCtxMenu->setAnnotationHere(annotationAtPos);
}

void DecompilerWidget::decompilerSelected()
{
    Config()->setSelectedDecompiler(ui->decompilerComboBox->currentData().toString());
    doRefresh();
}

void DecompilerWidget::connectCursorPositionChanged(bool connectPositionChange)
{
    if (!connectPositionChange) {
        disconnect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this,
                   &DecompilerWidget::cursorPositionChanged);
    } else {
        connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this,
                &DecompilerWidget::cursorPositionChanged);
    }
}

void DecompilerWidget::cursorPositionChanged()
{
    // Do not perform seeks along with the cursor while selecting multiple lines
    if (!ui->textEdit->textCursor().selectedText().isEmpty()) {
        return;
    }

    const size_t pos = ui->textEdit->textCursor().position();
    setAnnotationsAtCursor(pos);
    setInfoForBreakpoints();

    const RVA offset = offsetForPosition(pos);
    if (offset != RVA_INVALID && offset != seekable->getOffset()) {
        seekFromCursor = true;
        seekable->seek(offset);
        mCtxMenu->setOffset(offset);
        seekFromCursor = false;
    }
    updateSelection();
}

void DecompilerWidget::seekChanged(RVA /* addr */, CutterCore::SeekHistoryType type)
{
    if (seekFromCursor) {
        return;
    }

    if (!scrollHistory.empty()) { // History is empty upon init.
        scrollHistory[historyPos] = { ui->textEdit->horizontalScrollBar()->sliderPosition(),
                                      ui->textEdit->verticalScrollBar()->sliderPosition() };
    }
    if (type == CutterCore::SeekHistoryType::New) {
        // Erase previous history past this point.
        if (scrollHistory.size() > historyPos + 1) {
            scrollHistory.erase(scrollHistory.begin() + historyPos + 1, scrollHistory.end());
        }
        scrollHistory.push_back({ 0, 0 });
        historyPos = scrollHistory.size() - 1;
    } else if (type == CutterCore::SeekHistoryType::Undo) {
        --historyPos;
    } else if (type == CutterCore::SeekHistoryType::Redo) {
        ++historyPos;
    }
    const RVA fcnAddr = Core()->getFunctionStart(seekable->getOffset());
    if (fcnAddr == RVA_INVALID || fcnAddr != decompiledFunctionAddr) {
        doRefresh();
        return;
    }
    updateCursorPosition();
}

void DecompilerWidget::updateCursorPosition()
{
    const RVA offset = seekable->getOffset();
    const size_t pos = positionForOffset(offset);
    if (pos == SIZE_MAX) {
        return;
    }
    mCtxMenu->setOffset(offset);
    connectCursorPositionChanged(false);
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(pos);
    ui->textEdit->setTextCursor(cursor);
    updateSelection();
    connectCursorPositionChanged(true);
}

void DecompilerWidget::setupFonts()
{
    ui->textEdit->setFont(Config()->getFont());
}

void DecompilerWidget::updateSelection()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Highlight the current line
    QTextCursor cursor = ui->textEdit->textCursor();
    extraSelections.append(createLineHighlightSelection(cursor));

    // Highlight all the words in the document same as the current one
    cursor.select(QTextCursor::WordUnderCursor);
    const QString searchString = cursor.selectedText();
    mCtxMenu->setCurHighlightedWord(searchString);
    extraSelections.append(createSameWordsSelections(ui->textEdit, searchString));

    ui->textEdit->setExtraSelections(extraSelections);
    // Highlight PC after updating the selected line
    highlightPC();
}

QString DecompilerWidget::getWindowTitle() const
{
    const RzAnalysisFunction *fcn = Core()->functionAt(decompiledFunctionAddr);
    QString windowTitle;
    if (fcn != nullptr) {
        windowTitle = tr("Decompiler (%1)").arg(fcn->name);
    } else {
        windowTitle = tr("Decompiler (Empty)");
    }
    return windowTitle;
}

void DecompilerWidget::fontsUpdatedSlot()
{
    setupFonts();
}

void DecompilerWidget::colorsUpdatedSlot()
{
    const bool useAnotationHiglighter = Config()->isDecompilerAnnotationHighlighterEnabled();
    if (useAnotationHiglighter != usingAnnotationBasedHighlighting) {
        setHighlighter(useAnotationHiglighter);
    }
}

void DecompilerWidget::showDecompilerContextMenu(const QPoint &pt)
{
    mCtxMenu->exec(ui->textEdit->mapToGlobal(pt));
}

void DecompilerWidget::seekToReference()
{
    const size_t pos = ui->textEdit->textCursor().position();
    const RVA offset = findReference(pos);
    if (offset != RVA_INVALID) {
        seekable->seek(offset);
    }
    seekable->seekToReference(offsetForPosition(pos));
}

bool DecompilerWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (obj == ui->textEdit || obj == ui->textEdit->viewport())) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        ui->textEdit->setTextCursor(ui->textEdit->cursorForPosition(mouseEvent->pos()));
        seekToReference();
        return true;
    }
    if (event->type() == QEvent::MouseButtonPress
        && (obj == ui->textEdit || obj == ui->textEdit->viewport())) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton && !ui->textEdit->textCursor().hasSelection()) {
            ui->textEdit->setTextCursor(ui->textEdit->cursorForPosition(mouseEvent->pos()));
            return true;
        }
    }
    return MemoryDockWidget::eventFilter(obj, event);
}

void DecompilerWidget::highlightPC()
{
    const RVA pcAddress = Core()->getProgramCounterValue();
    if (pcAddress == RVA_INVALID
        || (Core()->getFunctionStart(pcAddress) != decompiledFunctionAddr)) {
        return;
    }

    const QTextCursor cursor = getCursorForAddress(pcAddress);
    if (!cursor.isNull()) {
        colorLine(createLineHighlightPC(cursor));
    }
}

void DecompilerWidget::highlightBreakpoints()
{

    const QList<RVA> functionBreakpoints = Core()->getBreakpointsInFunction(decompiledFunctionAddr);
    QTextCursor cursor;
    for (const RVA &bp : functionBreakpoints) {
        if (bp == RVA_INVALID) {
            continue;
        }
        cursor = getCursorForAddress(bp);
        if (!cursor.isNull()) {
            // Use a Block formatting since these lines are not updated frequently as selections and
            // PC
            QTextBlockFormat f;
            f.setBackground(ConfigColor("gui.breakpoint_background"));
            cursor.setBlockFormat(f);
        }
    }
}

bool DecompilerWidget::colorLine(const QTextEdit::ExtraSelection &extraSelection)
{
    QList<QTextEdit::ExtraSelection> extraSelections = ui->textEdit->extraSelections();
    extraSelections.append(extraSelection);
    ui->textEdit->setExtraSelections(extraSelections);
    return true;
}

void DecompilerWidget::copy()
{
    if (ui->textEdit->textCursor().hasSelection()) {
        ui->textEdit->copy();
    } else {
        QTextCursor cursor = ui->textEdit->textCursor();
        QClipboard *clipboard = QApplication::clipboard();
        cursor.select(QTextCursor::WordUnderCursor);
        if (!cursor.selectedText().isEmpty()) {
            clipboard->setText(cursor.selectedText());
        } else {
            cursor.select(QTextCursor::LineUnderCursor);
            clipboard->setText(cursor.selectedText());
        }
    }
}

bool DecompilerWidget::addressInRange(RVA addr) const
{
    if (lowestOffsetInCode <= addr && addr <= highestOffsetInCode) {
        return true;
    }
    return false;
}

/**
 * Convert annotation ranges from byte offsets in utf8 used by RzAnnotated code to QString QChars
 * used by QString and Qt text editor.
 * @param code - RzAnnotated code with annotations that need to be modified
 * @return Decompiled code
 */
static QString remapAnnotationOffsetsToQString(RzAnnotatedCode &code)
{
    const QByteArray bytes(code.code);
    std::vector<size_t> offsets;
    offsets.reserve(bytes.size());
    char c;
    for (size_t off = 0; c = code.code[off], c; off++) {
        if ((c & 0xc0) == 0x80) {
            continue;
        }
        offsets.push_back(off);
    }
    auto mapPos = [&](size_t pos) {
        auto it = std::upper_bound(offsets.begin(), offsets.end(), pos);
        if (it != offsets.begin()) {
            --it;
        }
        return it - offsets.begin();
    };

    RzCodeAnnotation *annotation;
    CutterRzVectorForeach(&code.annotations, annotation, RzCodeAnnotation)
    {
        annotation->start = mapPos(annotation->start);
        annotation->end = mapPos(annotation->end);
    }
    return QString::fromUtf8(code.code);
}

void DecompilerWidget::setCode(RzAnnotatedCode *code)
{
    connectCursorPositionChanged(false);
    if (auto highlighter = qobject_cast<DecompilerHighlighter *>(syntaxHighlighter.get())) {
        highlighter->setAnnotations(code);
    }
    this->code.reset(code);
    const QString text = remapAnnotationOffsetsToQString(*this->code);
    this->ui->textEdit->setPlainText(text);
    connectCursorPositionChanged(true);
    syntaxHighlighter->rehighlight();
}

void DecompilerWidget::setHighlighter(bool annotationBasedHighlighter)
{
    usingAnnotationBasedHighlighting = annotationBasedHighlighter;
    if (usingAnnotationBasedHighlighting) {
        syntaxHighlighter.reset(new DecompilerHighlighter());
        static_cast<DecompilerHighlighter *>(syntaxHighlighter.get())->setAnnotations(code.get());
    } else {
        syntaxHighlighter.reset(Config()->createSyntaxHighlighter(nullptr));
    }
    syntaxHighlighter->setDocument(ui->textEdit->document());
}
