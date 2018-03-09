#include "PseudocodeWidget.h"
#include "ui_PseudocodeWidget.h"

#include <QTextEdit>

#include "utils/Configuration.h"
#include "utils/Helpers.h"
#include "utils/SyntaxHighlighter.h"
#include "utils/TempConfig.h"

PseudocodeWidget::PseudocodeWidget(QWidget *parent, Qt::WindowFlags flags) :
        QDockWidget(parent, flags),
        ui(new Ui::PseudocodeWidget)
{
    ui->setupUi(this);

    syntaxHighLighter = new SyntaxHighlighter(ui->textEdit->document());

    setupFonts();
    colorsUpdatedSlot();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

    //connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshPseudocode()));
    //connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshPseudocode()));
    //connect(Core(), SIGNAL(functionRenamed(QString, QString)), this, SLOT(refreshPseudocode()));
    //connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshPseudocode()));
    //connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshPseudocode()));
    //connect(Core(), &CutterCore::instructionChanged, this, [this](/*RVA offset*/) {
    //        refreshPseudocode();
    //});


    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this, SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));
    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility)
        {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Pseudocode);
        }
    });

    connect(ui->refreshButton, &QAbstractButton::clicked, this, [this]() {
        refresh(Core()->getOffset());
    });

    refresh(RVA_INVALID);
}

PseudocodeWidget::PseudocodeWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
        : PseudocodeWidget(parent, flags)
{
    setWindowTitle(title);
}

PseudocodeWidget::~PseudocodeWidget() {}


void PseudocodeWidget::refresh(RVA addr)
{
    if (addr == RVA_INVALID)
    {
        ui->textEdit->setText(tr("Click Refresh to generate Pseudocode from current offset."));
        return;
    }

    const QString& decompiledCode = Core()->getDecompiledCode(addr);
    if (decompiledCode.length() == 0)
    {
        ui->textEdit->setText(tr("Cannot decompile at") + " " + RAddressString(addr) + " " + tr("(Not a function?)"));
        return;
    }
    ui->textEdit->setText(decompiledCode);
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
    ui->textEdit->setFont(font);
}

void PseudocodeWidget::fontsUpdated()
{
    setupFonts();
}

void PseudocodeWidget::colorsUpdatedSlot()
{
    const QString textEditClassName(ui->textEdit->metaObject()->className());
    QString styleSheet = QString(textEditClassName + " { background-color: %1; color: %2; }")
            .arg(ConfigColor("gui.background").name())
            .arg(ConfigColor("btext").name());
    ui->textEdit->setStyleSheet(styleSheet);
}
