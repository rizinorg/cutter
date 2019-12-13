#include "DecompilerWidget.h"
#include "ui_DecompilerWidget.h"
#include "menus/DisassemblyContextMenu.h"

#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "common/SelectionHighlight.h"
#include "common/Decompiler.h"

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QObject>
#include <QTextBlockUserData>

DecompilerWidget::DecompilerWidget(MainWindow *main, QAction *action) :
    MemoryDockWidget(MemoryWidgetType::Decompiler, main, action),
    mCtxMenu(new DisassemblyContextMenu(this, main)),
    ui(new Ui::DecompilerWidget)
{
    ui->setupUi(this);

    syntaxHighlighter = Config()->createSyntaxHighlighter(ui->textEdit->document());

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

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