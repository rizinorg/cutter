#include "PseudocodeWidget.h"
#include "ui_PseudocodeWidget.h"

#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"

#include <QTextEdit>

PseudocodeWidget::PseudocodeWidget(MainWindow *main, QAction *action) :
    MemoryDockWidget(CutterCore::MemoryWidgetType::Pseudocode, main, action),
    ui(new Ui::PseudocodeWidget)
{
    ui->setupUi(this);

    syntaxHighlighter = Config()->createSyntaxHighlighter(ui->textEdit->document());

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Pseudocode);
        }
    });

    // TODO Use RefreshDeferrer and remove the refresh button
    connect(ui->refreshButton, &QAbstractButton::clicked, this, [this]() {
        doRefresh(Core()->getOffset());
    });

    if (Core()->getR2DecAvailable()) {
        ui->decompilerComboBox->setEnabled(true);
        ui->decompilerComboBox->setCurrentIndex(DecompilerCBR2Dec);
    } else {
        ui->decompilerComboBox->setEnabled(false);
        ui->decompilerComboBox->setCurrentIndex(DecompilerCBPdc);
    }

    doRefresh(RVA_INVALID);
}

PseudocodeWidget::~PseudocodeWidget() = default;


void PseudocodeWidget::doRefresh(RVA addr)
{
    if (addr == RVA_INVALID) {
        ui->textEdit->setText(tr("Click Refresh to generate Pseudocode from current offset."));
        return;
    }

    QString decompiledCode;
    switch (ui->decompilerComboBox->currentIndex()) {
    case DecompilerCBR2Dec:
        if (Core()->getR2DecAvailable()) {
            decompiledCode = Core()->getDecompiledCodeR2Dec(addr);
            break;
        } // else fallthrough
    case DecompilerCBPdc:
    default:
        decompiledCode = Core()->getDecompiledCodePDC(addr);
        break;
    }

    if (decompiledCode.length() == 0) {
        ui->textEdit->setText(tr("Cannot decompile at") + " " + RAddressString(
                                  addr) + " " + tr("(Not a function?)"));
        return;
    }
    ui->textEdit->setText(decompiledCode);
}

void PseudocodeWidget::refreshPseudocode()
{
    doRefresh(Core()->getOffset());
}

void PseudocodeWidget::setupFonts()
{
    QFont font = Config()->getFont();
    ui->textEdit->setFont(font);
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
