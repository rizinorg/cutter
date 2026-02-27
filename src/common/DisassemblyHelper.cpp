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

RVA DisassemblyHelper::readDisassemblyArrow(QTextCursor tc)
{
    auto userData = getUserData(tc.block());
    if (!userData) {
        return RVA_INVALID;
    }

    return userData->line.arrow;
}

DisassemblyHelper::TargetContext DisassemblyHelper::getContextFromCursor(QTextCursor tc)
{
    tc.select(QTextCursor::WordUnderCursor);
    TargetContext ctx;
    ctx.word = tc.selectedText();
    ctx.line = tc.block().text();
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
        bool showXRefComments = Core()->getConfigb("asm.xrefs");
        if (showXRefComments && isXRefFromComment(ctx.offset, ctx.line)) {
            res.offset = getXRefFromWord(ctx.offset, ctx.word);
            res.type = TargetType::XRefComment;
            return res;
        }
    }

    if (filter & TargetFilter::Variables) {
        XrefDescription xref = Core()->getFirstXRefForVariable(ctx.word, ctx.offset);
        if (!xref.from_str.isEmpty() || !xref.to_str.isEmpty()) {
            res.offset = xref.from;
            res.type = TargetType::VariableName;
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
            res.offset = ctx.arrow;
            return res;
        }
    }

    return res;
}
