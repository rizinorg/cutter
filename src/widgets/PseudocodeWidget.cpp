#include "PseudocodeWidget.h"

#include <QTextEdit>

#include "utils/Configuration.h"
#include "utils/Helpers.h"
#include "utils/SyntaxHighlighter.h"
#include "utils/TempConfig.h"

PseudocodeWidget::PseudocodeWidget(QWidget *parent, Qt::WindowFlags flags)
    :   QDockWidget(parent, flags)
    ,   textEditWidget(new QTextEdit(this))
    ,   syntaxHighLighter( new SyntaxHighlighter(textEditWidget->document()))
{
    setObjectName("PseudocodeWidget");

    textEditWidget->setParent(this);
    setWidget(textEditWidget);
    setupFonts();
    colorsUpdatedSlot();
    textEditWidget->setReadOnly(true);
    textEditWidget->setLineWrapMode(QTextEdit::NoWrap);

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
    const QString& decompiledCode = Core()->getDecompiledCode(addr);
    if (decompiledCode.length() == 0)
    {
        textEditWidget->setText(tr("Cannot decompile at") + " " + RAddressString(addr) + " " + tr("(Not a function?)"));
        return;
    }
    textEditWidget->setText(decompiledCode);
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
    textEditWidget->setFont(font);
}

void PseudocodeWidget::fontsUpdated()
{
    setupFonts();
}

void PseudocodeWidget::colorsUpdatedSlot()
{
    const QString textEditClassName(textEditWidget->metaObject()->className());
    QString styleSheet = QString(textEditClassName + " { background-color: %1; color: %2; }")
            .arg(ConfigColor("gui.background").name())
            .arg(ConfigColor("btext").name());
    textEditWidget->setStyleSheet(styleSheet);
}
