#include "DecompilerWidget.h"
#include "ui_DecompilerWidget.h"
#include "menus/DisassemblyContextMenu.h"

#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "common/SelectionHighlight.h"
#include "common/Decompiler.h"
#include "common/CutterSeekable.h"

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QObject>
#include <QTextBlockUserData>

DecompilerWidget::DecompilerWidget(MainWindow *main) :
    MemoryDockWidget(MemoryWidgetType::Decompiler, main),
    mCtxMenu(new DisassemblyContextMenu(this, main)),
    ui(new Ui::DecompilerWidget)
{
    ui->setupUi(this);

    syntaxHighlighter = Config()->createSyntaxHighlighter(ui->textEdit->document());

    // Event filter to intercept double clicks in the textbox
    ui->textEdit->viewport()->installEventFilter(this);

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    connect(Core(), SIGNAL(registersChanged()), this, SLOT(highlightPC()));

    decompiledFunctionAddr = RVA_INVALID;
    decompilerWasBusy = false;

    connect(ui->refreshButton, &QAbstractButton::clicked, this, [this]() {
        doRefresh();
    });

    refreshDeferrer = createRefreshDeferrer([this]() {
        doRefresh();
    });

    autoRefreshEnabled = Config()->getDecompilerAutoRefreshEnabled();
    ui->autoRefreshCheckBox->setChecked(autoRefreshEnabled);
    setAutoRefresh(autoRefreshEnabled);
    connect(ui->autoRefreshCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
        setAutoRefresh(state == Qt::Checked);
        Config()->setDecompilerAutoRefreshEnabled(autoRefreshEnabled);
        doAutoRefresh();
    });

    auto decompilers = Core()->getDecompilers();
    auto selectedDecompilerId = Config()->getSelectedDecompiler();
    if (selectedDecompilerId.isEmpty()) {
        // If no decompiler was previously chosen. set r2ghidra as default decompiler
        selectedDecompilerId = "r2ghidra";
    }

    for (auto dec : decompilers) {
        ui->decompilerComboBox->addItem(dec->getName(), dec->getId());
        if (dec->getId() == selectedDecompilerId) {
            ui->decompilerComboBox->setCurrentIndex(ui->decompilerComboBox->count() - 1);
        }
        connect(dec, &Decompiler::finished, this, &DecompilerWidget::decompilationFinished);
    }

    decompilerSelectionEnabled = decompilers.size() > 1;
    ui->decompilerComboBox->setEnabled(decompilerSelectionEnabled);

    if (decompilers.isEmpty()) {
        ui->textEdit->setPlainText(tr("No Decompiler available."));
    }

    connect(ui->decompilerComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DecompilerWidget::decompilerSelected);
    connectCursorPositionChanged(false);
    connect(Core(), &CutterCore::seekChanged, this, &DecompilerWidget::seekChanged);
    ui->textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showDisasContextMenu(const QPoint &)));

    // refresh the widget when an action in this menu is triggered
    connect(mCtxMenu, &QMenu::triggered, this, &DecompilerWidget::refreshDecompiler);
    addActions(mCtxMenu->actions());

    ui->progressLabel->setVisible(false);
    doRefresh(RVA_INVALID);

    connect(Core(), &CutterCore::refreshAll, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::functionRenamed, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::varsChanged, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::functionsChanged, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::flagsChanged, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::commentsChanged, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::instructionChanged, this, &DecompilerWidget::doAutoRefresh);
    connect(Core(), &CutterCore::refreshCodeViews, this, &DecompilerWidget::doAutoRefresh);

    // Esc to seek backward
    QAction *seekPrevAction = new QAction(this);
    seekPrevAction->setShortcut(Qt::Key_Escape);
    seekPrevAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(seekPrevAction);
    connect(seekPrevAction, &QAction::triggered, seekable, &CutterSeekable::seekPrev);
}

DecompilerWidget::~DecompilerWidget() = default;

Decompiler *DecompilerWidget::getCurrentDecompiler()
{
    return Core()->getDecompilerById(ui->decompilerComboBox->currentData().toString());
}

void DecompilerWidget::setAutoRefresh(bool enabled)
{
    autoRefreshEnabled = enabled;
    updateRefreshButton();
}

void DecompilerWidget::doAutoRefresh()
{
    if (!autoRefreshEnabled) {
        return;
    }
    doRefresh();
}

void DecompilerWidget::updateRefreshButton()
{
    Decompiler *dec = getCurrentDecompiler();
    ui->refreshButton->setEnabled(!autoRefreshEnabled && dec && !dec->isRunning());
    if (dec && dec->isRunning() && dec->isCancelable()) {
        ui->refreshButton->setText(tr("Cancel"));
    } else {
        ui->refreshButton->setText(tr("Refresh"));
    }
}

void DecompilerWidget::doRefresh(RVA addr)
{
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

    if (dec->isRunning()) {
        decompilerWasBusy = true;
        return;
    }

    if (addr == RVA_INVALID) {
        ui->textEdit->setPlainText(tr("Click Refresh to generate Decompiler from current offset."));
        return;
    }

    // Clear all selections since we just refreshed
    ui->textEdit->setExtraSelections({});
    decompiledFunctionAddr = Core()->getFunctionStart(addr);
    dec->decompileAt(addr);
    if (dec->isRunning()) {
        ui->progressLabel->setVisible(true);
        ui->decompilerComboBox->setEnabled(false);
        updateRefreshButton();
        return;
    }
}

void DecompilerWidget::refreshDecompiler()
{
    doRefresh();
}

QTextCursor DecompilerWidget::getCursorForAddress(RVA addr)
{
    size_t pos = code.PositionForOffset(addr);
    if (pos == SIZE_MAX || pos == 0) {
        return QTextCursor();
    }

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(pos);
    return cursor;
}

void DecompilerWidget::decompilationFinished(AnnotatedCode code)
{
    ui->progressLabel->setVisible(false);
    ui->decompilerComboBox->setEnabled(decompilerSelectionEnabled);
    updateRefreshButton();

    this->code = code;
    if (code.code.isEmpty()) {
        ui->textEdit->setPlainText(tr("Cannot decompile at this address (Not a function?)"));
        return;
    } else {
        connectCursorPositionChanged(true);
        ui->textEdit->setPlainText(code.code);
        connectCursorPositionChanged(false);
        updateCursorPosition();
        highlightPC();
        highlightBreakpoints();
    }

    if (decompilerWasBusy) {
        decompilerWasBusy = false;
        doAutoRefresh();
    }
}

void DecompilerWidget::decompilerSelected()
{
    Config()->setSelectedDecompiler(ui->decompilerComboBox->currentData().toString());
    if (autoRefreshEnabled) {
        doRefresh();
    }
}

void DecompilerWidget::connectCursorPositionChanged(bool disconnect)
{
    if (disconnect) {
        QObject::disconnect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &DecompilerWidget::cursorPositionChanged);
    } else {
        connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &DecompilerWidget::cursorPositionChanged);
    }
}

void DecompilerWidget::cursorPositionChanged()
{
    // Do not perform seeks along with the cursor while selecting multiple lines
    if (!ui->textEdit->textCursor().selectedText().isEmpty())
    {
        return;
    }

    size_t pos = ui->textEdit->textCursor().position();
    RVA offset = code.OffsetForPosition(pos);
    if (offset != RVA_INVALID && offset != Core()->getOffset()) {
        seekFromCursor = true;
        Core()->seek(offset);
        mCtxMenu->setOffset(offset);
        seekFromCursor = false;
    }
    updateSelection();
}

void DecompilerWidget::seekChanged()
{
    if (seekFromCursor) {
        return;
    }

    if (autoRefreshEnabled) {
        auto fcnAddr = Core()->getFunctionStart(Core()->getOffset());
        if (fcnAddr == RVA_INVALID || fcnAddr != decompiledFunctionAddr) {
            doRefresh();
            return;
        }
    }

    updateCursorPosition();
}

void DecompilerWidget::updateCursorPosition()
{
    RVA offset = Core()->getOffset();
    size_t pos = code.PositionForOffset(offset);
    if (pos == SIZE_MAX) {
        return;
    }
    mCtxMenu->setOffset(offset);
    connectCursorPositionChanged(true);
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(pos);
    ui->textEdit->setTextCursor(cursor);
    updateSelection();
    connectCursorPositionChanged(false);
}

void DecompilerWidget::setupFonts()
{
    ui->textEdit->setFont(Config()->getFont());
}

void DecompilerWidget::updateSelection()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Highlight the current line
    auto cursor = ui->textEdit->textCursor();
    extraSelections.append(createLineHighlightSelection(cursor));

    // Highlight all the words in the document same as the current one
    cursor.select(QTextCursor::WordUnderCursor);
    QString searchString = cursor.selectedText();
    extraSelections.append(createSameWordsSelections(ui->textEdit, searchString));

    ui->textEdit->setExtraSelections(extraSelections);
    // Highlight PC after updating the selected line
    highlightPC();
    mCtxMenu->setCurHighlightedWord(searchString);
}

QString DecompilerWidget::getWindowTitle() const
{
    return tr("Decompiler");
}

void DecompilerWidget::fontsUpdatedSlot()
{
    setupFonts();
}

void DecompilerWidget::colorsUpdatedSlot()
{
}

void DecompilerWidget::showDisasContextMenu(const QPoint &pt)
{
    mCtxMenu->exec(ui->textEdit->mapToGlobal(pt));
    doRefresh();
}

void DecompilerWidget::seekToReference()
{
    size_t pos = ui->textEdit->textCursor().position();
    RVA offset = code.OffsetForPosition(pos);
    seekable->seekToReference(offset);
}

bool DecompilerWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (obj == ui->textEdit || obj == ui->textEdit->viewport())) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        const QTextCursor& cursor = ui->textEdit->cursorForPosition(QPoint(mouseEvent->x(), mouseEvent->y()));
        seekToReference();
        return true;
    }

    return MemoryDockWidget::eventFilter(obj, event);
}


void DecompilerWidget::highlightPC()
{
    RVA PCAddress = Core()->getProgramCounterValue();
    if (PCAddress == RVA_INVALID || (Core()->getFunctionStart(PCAddress) != decompiledFunctionAddr)) {
        return;
    }

    QTextCursor cursor = getCursorForAddress(PCAddress);
    if (!cursor.isNull()) {
        colorLine(createLineHighlightPC(cursor));
    }
    
}

void DecompilerWidget::highlightBreakpoints()
{

    QList<RVA> functionBreakpoints = Core()->getBreakpointsInFunction(decompiledFunctionAddr);
    QTextCursor cursor;
    foreach(auto &bp, functionBreakpoints) {
        if (bp == RVA_INVALID) {
            continue;;
        }

        cursor = getCursorForAddress(bp);
        if (!cursor.isNull()) {
            // Use a Block formatting since these lines are not updated frequently as selections and PC
            QTextBlockFormat f;
            f.setBackground(ConfigColor("gui.breakpoint_background"));
            cursor.setBlockFormat(f);
        }
    }
}

bool DecompilerWidget::colorLine(QTextEdit::ExtraSelection extraSelection)
{
    QList<QTextEdit::ExtraSelection> extraSelections = ui->textEdit->extraSelections();
    extraSelections.append(extraSelection);
    ui->textEdit->setExtraSelections(extraSelections);
    return true;
}
