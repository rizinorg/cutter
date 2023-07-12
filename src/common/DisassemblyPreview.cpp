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
    QList<XrefDescription> refs = Core()->getXRefs(offsetFrom, false, false);
    if (refs.length()) {
        if (refs.length() > 1) {
            qWarning() << QObject::tr(
                                  "More than one (%1) references here. Weird behaviour expected.")
                                  .arg(refs.length());
        }

        RVA offsetTo = refs.at(0).to; // This is the offset we want to preview
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

                QToolTip::showText(pointOfEvent, tooltip, parent, QRect {}, 3500);
                return true;
            }
        }
    }
    return false;
}

RVA DisassemblyPreview::readDisassemblyOffset(QTextCursor tc)
{
    auto userData = getUserData(tc.block());
    if (!userData) {
        return RVA_INVALID;
    }

    return userData->line.offset;
}


typedef struct mmio_lookup_context {
    QString selected;
    RVA mmio_address;
} mmio_lookup_context_t;


static bool lookup_mmio_addr_cb(void *user, const ut64 key, const void *value) {
    mmio_lookup_context_t* ctx = (mmio_lookup_context_t*) user;
    if (ctx->selected == (const char*)value) {
        ctx->mmio_address = key;
        return false;
    }
    return true;
}

bool DisassemblyPreview::showDebugValueTooltip(QWidget *parent, const QPoint &pointOfEvent, const QString &selectedText, const RVA offset)
{
    if (selectedText.isEmpty())
        return false;

    if (selectedText.at(0).isLetter()) {
        {
            // TODO: Lookup by name instead of loop
            const auto registerRefs = Core()->getRegisterRefValues();
            for (auto &reg : registerRefs) {
                if (reg.name == selectedText) {
                    auto msg = QString("%1=%2").arg(reg.name, reg.value);
                    QToolTip::showText(pointOfEvent, msg, parent);
                    return true;
                }
            }
        }

        if (offset != RVA_INVALID) {
            const auto vars = Core()->getVariables(offset);
            for (auto &var : vars) {
                if (var.name == selectedText) {
                    // TODO: Value/refs
                    auto msg = QString("%1: %2").arg(var.name, var.type);
                    QToolTip::showText(pointOfEvent, msg, parent);
                    return true;
                }
            }
        }

        {
            // Lookup MMIO address
            RzPlatformTarget *arch_target = Core()->core()->analysis->arch_target;
            if (arch_target && arch_target->profile) {
                mmio_lookup_context_t ctx;
                ctx.selected = selectedText;
                ctx.mmio_address = 0;
                ht_up_foreach(arch_target->profile->registers_mmio, lookup_mmio_addr_cb, &ctx);
                if (ctx.mmio_address) {
                    auto msg = QString("%1 (0x%2)").arg(selectedText, QString::number(ctx.mmio_address, 16));
                    QToolTip::showText(pointOfEvent, msg, parent);
                    return true;
                }
            }
        }
    }
    return false;
}
