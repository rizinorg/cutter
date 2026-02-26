#include "DisassemblyHelper.h"
#include "Cutter.h"

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
    RVA selectedOffset = Core()->num(selectedWord);
    auto xrefsTo = Core()->getXRefs(offset, true, false);
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

RVA DisassemblyHelper::readDisassemblyOffset(QTextCursor tc)
{
    auto userData = DisassemblyHelper::getUserData(tc.block());
    if (!userData) {
        return RVA_INVALID;
    }

    return userData->line.offset;
}

DisassemblyHelper::TargetContext DisassemblyHelper::getContextFromCursor(QTextCursor tc)
{
    tc.select(QTextCursor::WordUnderCursor);
    TargetContext ctx;
    ctx.word = tc.selectedText();
    ctx.line = tc.block().text();
    ctx.offset = DisassemblyHelper::readDisassemblyOffset(tc);
    return ctx;
}

DisassemblyHelper::TargetAction DisassemblyHelper::resolveTarget(const TargetContext &ctx,
                                                                 TargetFilter filter)
{
    TargetAction res = { RVA_INVALID, TargetType::None };

    if (filter == TargetFilter::All || filter == TargetFilter::XRefCommentOnly) {
        // Xref comments need special handling to show preview for each caller offset
        bool showXRefComments = Core()->getConfigb("asm.xrefs");
        if (showXRefComments && isXRefFromComment(ctx.offset, ctx.line)) {
            res.offset = getXRefFromWord(ctx.offset, ctx.word);
            res.type = TargetType::XRefComment;
            return res;
        }
    }

    if (filter == TargetFilter::All) {
        // check for local variables
        XrefDescription xref = Core()->getFirstXRefForVariable(ctx.word, ctx.offset);
        if (!xref.from_str.isEmpty() || !xref.to_str.isEmpty()) {
            res.offset = xref.from;
            res.type = TargetType::VariableName;
            return res;
        }

        // check for types
        if (Core()->typeExists(ctx.word)) {
            res.type = TargetType::TypeName;
            return res;
        }
    }

    return res;
}
