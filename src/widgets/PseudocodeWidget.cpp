#include "PseudocodeWidget.h"
#include "ui_PseudocodeWidget.h"
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

PseudocodeWidget::PseudocodeWidget(MainWindow *main, QAction *action) :
    MemoryDockWidget(MemoryWidgetType::Pseudocode, main, action),
    mCtxMenu(new DisassemblyContextMenu(this, main)),
    ui(new Ui::PseudocodeWidget)
{
    ui->setupUi(this);

    syntaxHighlighter = Config()->createSyntaxHighlighter(ui->textEdit->document());

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));
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
        connect(dec, &Decompiler::finished, this, &PseudocodeWidget::decompilationFinished);
    }

    decompilerSelectionEnabled = decompilers.size() > 1;
    ui->decompilerComboBox->setEnabled(decompilerSelectionEnabled);

    if (decompilers.isEmpty()) {
        ui->textEdit->setPlainText(tr("No Decompiler available."));
    }

    connect(ui->decompilerComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PseudocodeWidget::decompilerSelected);
    connectCursorPositionChanged(false);
    connect(Core(), &CutterCore::seekChanged, this, &PseudocodeWidget::seekChanged);
    ui->textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showDisasContextMenu(const QPoint &)));

    // refresh the widget when an action in this menu is triggered
    connect(mCtxMenu, &QMenu::triggered, this, &PseudocodeWidget::refreshPseudocode);
    addActions(mCtxMenu->actions());

    ui->progressLabel->setVisible(false);
    doRefresh(RVA_INVALID);

    connect(Core(), &CutterCore::refreshAll, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::functionRenamed, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::varsChanged, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::functionsChanged, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::flagsChanged, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::commentsChanged, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::instructionChanged, this, &PseudocodeWidget::doAutoRefresh);
    connect(Core(), &CutterCore::refreshCodeViews, this, &PseudocodeWidget::doAutoRefresh);
}

PseudocodeWidget::~PseudocodeWidget() = default;

Decompiler *PseudocodeWidget::getCurrentDecompiler()
{
    return Core()->getDecompilerById(ui->decompilerComboBox->currentData().toString());
}

void PseudocodeWidget::setAutoRefresh(bool enabled)
{
    autoRefreshEnabled = enabled;
    updateRefreshButton();
}

void PseudocodeWidget::doAutoRefresh()
{
    if (!autoRefreshEnabled) {
        return;
    }
    doRefresh();
}

void PseudocodeWidget::updateRefreshButton()
{
    Decompiler *dec = getCurrentDecompiler();
    ui->refreshButton->setEnabled(!autoRefreshEnabled && dec && !dec->isRunning());
    if (dec && dec->isRunning() && dec->isCancelable()) {
        ui->refreshButton->setText(tr("Cancel"));
    } else {
        ui->refreshButton->setText(tr("Refresh"));
    }
}

void PseudocodeWidget::doRefresh(RVA addr)
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
        ui->textEdit->setPlainText(tr("Click Refresh to generate Pseudocode from current offset."));
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

void PseudocodeWidget::refreshPseudocode()
{
    doRefresh();
}

void PseudocodeWidget::decompilationFinished(AnnotatedCode code)
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

void PseudocodeWidget::decompilerSelected()
{
    Config()->setSelectedDecompiler(ui->decompilerComboBox->currentData().toString());
    if (autoRefreshEnabled) {
        doRefresh();
    }
}

void PseudocodeWidget::connectCursorPositionChanged(bool disconnect)
{
    if (disconnect) {
        QObject::disconnect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &PseudocodeWidget::cursorPositionChanged);
    } else {
        connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &PseudocodeWidget::cursorPositionChanged);
    }
}

void PseudocodeWidget::cursorPositionChanged()
{
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

void PseudocodeWidget::seekChanged()
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

void PseudocodeWidget::updateCursorPosition()
{
    RVA offset = Core()->getOffset();
    size_t pos = code.PositionForOffset(offset);
    if (pos == SIZE_MAX) {
        return;
    }
    connectCursorPositionChanged(true);
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(pos);
    ui->textEdit->setTextCursor(cursor);
    updateSelection();
    connectCursorPositionChanged(false);
}

void PseudocodeWidget::setupFonts()
{
    QFont font = Config()->getFont();
    ui->textEdit->setFont(font);
}

void PseudocodeWidget::updateSelection()
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

QString PseudocodeWidget::getWindowTitle() const
{
    return tr("Pseudocode");
}

void PseudocodeWidget::fontsUpdated()
{
    setupFonts();
}

void PseudocodeWidget::colorsUpdatedSlot()
{
}

void PseudocodeWidget::showDisasContextMenu(const QPoint &pt)
{
    mCtxMenu->exec(ui->textEdit->mapToGlobal(pt));
    doRefresh();
}