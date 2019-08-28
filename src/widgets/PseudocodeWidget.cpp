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

    // TODO Use RefreshDeferrer and remove the refresh button
    connect(ui->refreshButton, &QAbstractButton::clicked, this, [this]() {
        doRefresh(Core()->getOffset());
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
}

PseudocodeWidget::~PseudocodeWidget() = default;


void PseudocodeWidget::doRefresh(RVA addr)
{
    if (ui->decompilerComboBox->currentIndex() < 0) {
        return;
    }

    Decompiler *dec = Core()->getDecompilerById(ui->decompilerComboBox->currentData().toString());
    if (!dec || dec->isRunning()) {
        return;
    }

    if (addr == RVA_INVALID) {
        ui->textEdit->setPlainText(tr("Click Refresh to generate Pseudocode from current offset."));
        return;
    }

    dec->decompileAt(addr);
    if (dec->isRunning()) {
        ui->progressLabel->setVisible(true);
        ui->decompilerComboBox->setEnabled(false);
        if (dec->isCancelable()) {
            ui->refreshButton->setText(tr("Cancel"));
        } else {
            ui->refreshButton->setEnabled(false);
        }
        return;
    }
}

void PseudocodeWidget::refreshPseudocode()
{
    doRefresh(Core()->getOffset());
}

void PseudocodeWidget::decompilationFinished(AnnotatedCode code)
{
    ui->progressLabel->setVisible(false);
    ui->decompilerComboBox->setEnabled(decompilerSelectionEnabled);
    ui->refreshButton->setText(tr("Refresh"));
    ui->refreshButton->setEnabled(true);

    this->code = code;
    if (code.code.isEmpty()) {
        ui->textEdit->setPlainText(tr("Cannot decompile at this address (Not a function?)"));
        return;
    } else {
        connectCursorPositionChanged(true);
        ui->textEdit->setPlainText(code.code);
        connectCursorPositionChanged(false);
        seekChanged();
    }
}

void PseudocodeWidget::decompilerSelected()
{
    Config()->setSelectedDecompiler(ui->decompilerComboBox->currentData().toString());
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
    doRefresh(Core()->getOffset());
}