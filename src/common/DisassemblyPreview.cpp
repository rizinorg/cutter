#include "DisassemblyPreview.h"

#include "Configuration.h"
#include "rz_types_base.h"

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QToolTip>
#include <QWidget>

namespace DH = DisassemblyHelper;

QString DisassemblyPreview::getToolTipStyleSheet()
{
    return QString { "QToolTip { border-width: 1px; max-width: %1px;"
                     "opacity: 230; background-color: %2;"
                     "color: %3; border-color: %3;}" }
            .arg(QString::number(400), Config()->getColor("gui.tooltip.background").name(),
                 Config()->getColor("gui.tooltip.foreground").name());
}

bool DisassemblyPreview::showDisasPreview(QWidget *parent, const QPoint &pointOfEvent,
                                          const RVA offsetFrom)
{
    const QList<XrefDescription> refs = Core()->getXRefs(offsetFrom, false, false);
    if (refs.length()) {
        if (refs.length() > 1) {
            qWarning() << QObject::tr(
                                  "More than one (%1) references here. Weird behaviour expected.")
                                  .arg(refs.length());
        }

        const RVA offsetTo = refs.at(0).to; // This is the offset we want to preview
        /*
         * Only if the offset we point *to* is different from the one the cursor is currently
         * on *and* the former is a valid offset, we are allowed to get a preview of offsetTo
         */
        if (offsetTo != offsetFrom && offsetTo != RVA_INVALID) {
            return showDisasPreviewAt(parent, pointOfEvent, offsetTo);
        }
    }
    return false;
}

bool DisassemblyPreview::showDisasPreviewAt(QWidget *parent, const QPoint &pointOfEvent,
                                            const RVA offset)
{
    const QStringList disasmPreview = Core()->getDisassemblyPreview(offset, 10);
    if (!disasmPreview.isEmpty()) {
        const QFont &fnt = Config()->getFont();
        const QString tooltip = QString("<html><div style=\"font-family: %1; font-size: %2pt; "
                                        "white-space: nowrap;\"><div style=\"margin-bottom: "
                                        "10px;\"><strong>Disassembly Preview</strong>:<br>%3<div>")
                                        .arg(fnt.family())
                                        .arg(qMax(8, fnt.pointSize() - 1))
                                        .arg(disasmPreview.join("<br>"));

        QToolTip::showText(pointOfEvent, tooltip, parent, QRect {}, 3500);
        return true;
    }

    return false;
}

bool DisassemblyPreview::showDebugValueTooltip(QWidget *parent, const QPoint &pointOfEvent,
                                               const DH::TargetAction &ta,
                                               const DH::TargetContext &ctx)
{
    QString msg;
    switch (ta.type) {
    case DH::TargetType::Register: {
        msg = QString("reg %1 = 0x%2").arg(ctx.word).arg(ta.value, 0, 16);
        auto fcn = Core()->functionIn(ta.value);
        if (fcn) {
            msg += QString(" (%1)").arg(fcn->name);
        }
        break;
    }
    case DH::TargetType::VariableValue: {
        msg = QString("var %1 = 0x%2").arg(ctx.word).arg(ta.value, 0, 16);
        break;
    }
    case DH::TargetType::MMIO: {
        const int len = 8; // TODO: Determine proper len of mmio address for the cpu
        auto core = Core()->lock();
        if (char *r = rz_core_print_hexdump_or_hexdiff_str(core, RZ_OUTPUT_MODE_STANDARD, ta.value,
                                                           len, false)) {
            msg = QString("mmio %1 %2").arg(ctx.word, QString::fromUtf8(r).trimmed());
            free(r);
        }
        break;
    }
    case DH::TargetType::Memory: {
        const ut64 addr = Core()->math(ctx.word.mid(1, ctx.word.length() - 2));
        msg = QString("%1 = 0x%2 -> 0x%3").arg(ctx.word).arg(addr, 0, 16).arg(ta.value, 0, 16);
        break;
    }
    default:
        return false;
    }

    if (!msg.isEmpty()) {
        QToolTip::showText(pointOfEvent, msg, parent);
        return true;
    }

    // Else show preview for value?
    return false;
}

bool DisassemblyPreview::showTooltip(QWidget *parent, const QPoint &globalPos,
                                     const DH::TargetContext &ctx, bool hasPreview)
{
    const bool isWordEmpty = ctx.word.isEmpty();
    if (hasPreview) {
        auto ta = DH::resolveTarget(ctx, DH::XRefComments | DH::Arrows);

        if (!isWordEmpty && ta.type == DH::TargetType::XRefComment) {
            if (ta.value != RVA_INVALID) {
                showDisasPreviewAt(parent, globalPos, ta.value);
            }
            // consume the event even if the text under cursor is not an address, this prevents
            // jumping to incorrect offset (offset pointed to by the next instruction line)
            // when double clicking on auto generated XREF comment
            return true;
        }

        if (ta.type == DH::TargetType::Arrow && showDisasPreviewAt(parent, globalPos, ta.value)) {
            return true;
        }

        if (showDisasPreview(parent, globalPos, ctx.offset)) {
            return true;
        }
    }

    if (Config()->getShowVarTooltips() && (Core()->currentlyDebugging || Core()->currentlyEmulating)
        && !isWordEmpty) {
        auto ta = DH::resolveTarget(ctx, DH::Debug);
        if (ta.type != DH::TargetType::None && showDebugValueTooltip(parent, globalPos, ta, ctx)) {
            return true;
        }
    }

    return false;
}
