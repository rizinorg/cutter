#include "DisassemblyHelper.h"

#include "Cutter.h"
#include "rz_types_base.h"

typedef struct MmioLookupContext
{
    QString selected;
    RVA mmioAddress;
} mmio_lookup_context_t;

static bool lookupMmioAddrCb(void *user, const ut64 key, const void *value)
{
    auto *ctx = static_cast<mmio_lookup_context_t *>(user);
    if (ctx->selected == static_cast<const char *>(value)) {
        ctx->mmioAddress = key;
        return false;
    }
    return true;
}

DisassemblyTextBlockUserData::DisassemblyTextBlockUserData(const DisassemblyLine &line)
    : line { line }
{
}

DisassemblyTextBlockUserData *DisassemblyHelper::getUserData(const QTextBlock &block)
{
    QTextBlockUserData *userData = block.userData();
    if (!userData) {
        return nullptr;
    }

    return static_cast<DisassemblyTextBlockUserData *>(userData);
}

RVA DisassemblyHelper::getXRefFromWord(RVA offset, const QString &selectedWord)
{
    const RVA selectedOffset = Core()->num(selectedWord);
    const auto xrefsTo = Core()->getXRefs(offset, true, false);
    for (const auto &xref : xrefsTo) {
        if (xref.from == selectedOffset) {
            return xref.from;
        }
    }
    return RVA_INVALID;
}

bool DisassemblyHelper::isXRefFromComment(RVA offset, const QString &line)
{
    return Core()->getXRefCommentAt(offset).simplified().contains(line.simplified());
}

RVA DisassemblyHelper::readDisassemblyOffset(const QTextCursor &tc)
{
    auto userData = DisassemblyHelper::getUserData(tc.block());
    if (!userData) {
        return RVA_INVALID;
    }

    return userData->line.offset;
}

RVA DisassemblyHelper::readDisassemblyArrow(const QTextCursor &tc)
{
    auto userData = getUserData(tc.block());
    if (!userData) {
        return RVA_INVALID;
    }

    return userData->line.arrow;
}

DisassemblyHelper::BracketResult DisassemblyHelper::findBracketRange(const QString &line,
                                                                     int posInLine)
{
    BracketResult res;
    const int openBracket = line.lastIndexOf('[', posInLine);
    const int closeBracket = line.indexOf(']', posInLine);

    if (openBracket != -1 && closeBracket != -1) {
        bool isInside = true;
        for (int i = openBracket + 1; i < posInLine; ++i) {
            if (line[i] == ']') {
                isInside = false;
                break;
            }
        }
        if (isInside) {
            for (int i = posInLine; i < closeBracket; ++i) {
                if (line[i] == '[') {
                    isInside = false;
                    break;
                }
            }
        }

        if (isInside) {
            res.found = true;
            res.start = openBracket;
            res.length = (closeBracket - openBracket) + 1;
            res.content = line.mid(res.start, res.length);
        }
    }
    return res;
}

DisassemblyHelper::TargetContext DisassemblyHelper::getContextFromCursor(QTextCursor tc)
{
    const int originalPos = tc.position();
    tc.select(QTextCursor::WordUnderCursor);
    QString word = tc.selectedText();
    const QString line = tc.block().text();
    const int lineStart = tc.block().position();
    const int posInLine = originalPos - lineStart;

    auto bracketRes = findBracketRange(line, posInLine);
    if (bracketRes.found) {
        tc.setPosition(lineStart + bracketRes.start);
        tc.setPosition(lineStart + bracketRes.start + bracketRes.length, QTextCursor::KeepAnchor);
        word = bracketRes.content;
    }

    TargetContext ctx;
    ctx.word = word;
    ctx.line = line;
    ctx.offset = DisassemblyHelper::readDisassemblyOffset(tc);
    ctx.arrow = DisassemblyHelper::readDisassemblyArrow(tc);
    return ctx;
}

DisassemblyHelper::TargetAction DisassemblyHelper::resolveTarget(const TargetContext &ctx,
                                                                 int filter)
{
    TargetAction res = { RVA_INVALID, TargetType::None };

    // Xref comments need special handling to show preview for each caller offset
    if (filter & TargetFilter::XRefComments) {
        const bool showXRefComments = Core()->getConfigb("asm.xrefs");
        if (showXRefComments && isXRefFromComment(ctx.offset, ctx.line)) {
            res.value = getXRefFromWord(ctx.offset, ctx.word);
            res.type = TargetType::XRefComment;
            return res;
        }
    }

    if (filter & TargetFilter::VariableValues) {
        QString inner = ctx.word;
        if (inner.startsWith('[') && inner.endsWith(']')) {
            inner = inner.mid(1, inner.length() - 2);
        }

        if (ctx.offset != RVA_INVALID) {
            auto vars = Core()->getVariables(ctx.offset);
            for (auto &var : vars) {
                if (var.name == inner) {
                    res.value = Core()->math(var.value);
                    res.type = TargetType::VariableValue;
                    return res;
                }
            }
        }
    }

    if (filter & TargetFilter::VariableXrefs) {
        const XrefDescription xref = Core()->getFirstXRefForVariable(ctx.word, ctx.offset);
        if (!xref.fromStr.isEmpty() || !xref.toStr.isEmpty()) {
            res.value = xref.from;
            res.type = TargetType::VariableXRef;
            return res;
        }
    }

    if (filter & TargetFilter::Types) {
        if (Core()->typeExists(ctx.word)) {
            res.type = TargetType::TypeName;
            return res;
        }
    }

    if (filter & TargetFilter::Arrows) {
        if (ctx.arrow != RVA_INVALID) {
            res.type = TargetType::Arrow;
            res.value = ctx.arrow;
            return res;
        }
    }

    if (!ctx.word.isEmpty()) {
        if (filter & TargetFilter::Registers) {
            const auto reg = Core()->getRegisterRefValue(ctx.word);
            if (!reg.name.isEmpty()) {
                res.type = TargetType::Register;
                res.value = Core()->math(reg.value);
                return res;
            }
        }

        if (filter & TargetFilter::MMIO) {
            mmio_lookup_context_t mmioCtx;
            mmioCtx.selected = ctx.word;
            mmioCtx.mmioAddress = RVA_INVALID;
            auto core = Core()->lock();
            const RzPlatformTarget *archTarget = rz_analysis_get_arch_target(core->analysis);
            if (archTarget && archTarget->profile) {
                ht_up_foreach(archTarget->profile->registers_mmio, lookupMmioAddrCb, &mmioCtx);
            }
            if (mmioCtx.mmioAddress != RVA_INVALID) {
                res.value = mmioCtx.mmioAddress;
                res.type = TargetType::MMIO;
                return res;
            }
        }

        if (filter & TargetFilter::Memory) {
            QString stripped = ctx.word;
            if (stripped.startsWith('[') && stripped.endsWith(']')) {
                stripped = stripped.mid(1, stripped.length() - 2);
                if (Core()->isValidInputNumValue(stripped)
                    || Core()->isValidInputNumValue(ctx.word)) {
                    res.value = Core()->math(ctx.word);
                    res.type = TargetType::Memory;
                    return res;
                }
            }
        }
    }

    return res;
}
