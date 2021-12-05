#include "DisassemblyPreview.h"
#include "Configuration.h"
#include "widgets/GraphView.h"

#include <QCoreApplication>
#include <QWidget>
#include <QToolTip>
#include <QProcessEnvironment>

DisassemblyTextBlockUserData::DisassemblyTextBlockUserData(const DisassemblyLine &line)
    : line { line }
{
}

DisassemblyTextBlockUserData *getUserData(const QTextBlock &block)
{
    QTextBlockUserData *userData = block.userData();
    if (!userData) {
        return nullptr;
    }

    return static_cast<DisassemblyTextBlockUserData *>(userData);
}

QString DisassemblyPreview::getToolTipStyleSheet()
{
    return QString { "QToolTip { border-width: 1px; max-width: %1px;"
                     "opacity: 230; background-color: %2;"
                     "color: %3; border-color: %3;}" }
            .arg(400)
            .arg(Config()->getColor("gui.tooltip.background").name())
            .arg(Config()->getColor("gui.tooltip.foreground").name());
}

bool DisassemblyPreview::showDisasPreview(QWidget *parent, const QPoint &pointOfEvent,
                                          const RVA offsetFrom)
{
    QProcessEnvironment env;
    QPoint point = pointOfEvent;

    QList<XrefDescription> refs = Core()->getXRefs(offsetFrom, false, false);
    if (refs.length()) {
        if (refs.length() > 1) {
            qWarning() << QObject::tr(
                                  "More than one (%1) references here. Weird behaviour expected.")
                                  .arg(refs.length());
        }

        RVA offsetTo = refs.at(0).to; // This is the offset we want to preview

        if (Q_UNLIKELY(offsetFrom != refs.at(0).from)) {
            qWarning() << QObject::tr("offsetFrom (%1) differs from refs.at(0).from (%(2))")
                                  .arg(offsetFrom)
                                  .arg(refs.at(0).from);
        }

        /*
         * Only if the offset we point *to* is different from the one the cursor is currently
         * on *and* the former is a valid offset, we are allowed to get a preview of offsetTo
         */
        if (offsetTo != offsetFrom && offsetTo != RVA_INVALID) {
            QStringList disasmPreview = Core()->getDisassemblyPreview(offsetTo, 10);

            // Last check to make sure the returned preview isn't an empty text (QStringList)
            if (!disasmPreview.isEmpty()) {
                const QFont &fnt = Config()->getFont();

                QFontMetrics fm { fnt };

                QString tooltip =
                        QString { "<html><div style=\"font-family: %1; font-size: %2pt; "
                                  "white-space: nowrap;\"><div style=\"margin-bottom: "
                                  "10px;\"><strong>Disassembly Preview</strong>:<br>%3<div>" }
                                .arg(fnt.family())
                                .arg(qMax(8, fnt.pointSize() - 1))
                                .arg(disasmPreview.join("<br>"));

                QToolTip::showText(point, tooltip, parent, QRect {}, 3500);
                return true;
            }
        }
    }
    return false;
}
