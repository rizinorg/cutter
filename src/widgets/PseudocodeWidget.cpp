#include "PseudocodeWidget.h"
#include "ui_PseudocodeWidget.h"

#include "utils/Configuration.h"
#include "utils/Helpers.h"
#include "utils/TempConfig.h"

PseudocodeWidget::PseudocodeWidget(QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(parent, flags),
    ui(new Ui::PseudocodeWidget)
{
    ui->setupUi(this);

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshPseudocode()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshPseudocode()));
    connect(Core(), SIGNAL(functionRenamed(QString, QString)), this, SLOT(refreshPseudocode()));
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshPseudocode()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshPseudocode()));
    connect(Core(), &CutterCore::instructionChanged, this, [this](/*RVA offset*/) {
            refreshPseudocode();
    });




    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this, SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));
    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility)
        {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Pseudocode);
        }
    });

    // Get alerted when there's a refresh
    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refresh(Core()->getOffset());
    });

    refresh(Core()->getOffset());
}

PseudocodeWidget::PseudocodeWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
        : PseudocodeWidget(parent, flags)
{
    setWindowTitle(title);
}

PseudocodeWidget::~PseudocodeWidget() {}


void PseudocodeWidget::on_seekChanged(RVA addr)
{
    refresh(addr);
}

void PseudocodeWidget::refresh(RVA addr)
{
    QString decompiledCode = Core()->getDecompiledCode(addr);
    if (decompiledCode.length() == 0)
    {
        decompiledCode = tr("Cannot decompile at") + " " + RAddressString(addr) + " " + tr("(Not a function?)");
    }
    ui->pseudocodeTextBrowser->setText(decompiledCode);
}

void PseudocodeWidget::refreshPseudocode()
{
    refresh(Core()->getOffset());
}

void PseudocodeWidget::raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type)
{
    if (type == CutterCore::MemoryWidgetType::Pseudocode)
    {
        raise();
    }
}

void PseudocodeWidget::setupFonts()
{
    QFont font = Config()->getFont();
    ui->pseudocodeTextBrowser->setFont(font);
}

void PseudocodeWidget::fontsUpdated()
{
    setupFonts();
}

void PseudocodeWidget::colorsUpdatedSlot()
{
    QString styleSheet = QString("QTextBrowser { background-color: %1; color: %2; }")
            .arg(ConfigColor("gui.background").name())
            .arg(ConfigColor("btext").name());
    ui->pseudocodeTextBrowser->setStyleSheet(styleSheet);
}
