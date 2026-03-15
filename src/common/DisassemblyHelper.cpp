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

QString DisassemblyHelper::normalizeExpression(QString expr){
    expr = expr.trimmed();

    // remove surrounding brackets
    if (expr.startsWith('[') && expr.endsWith(']')) {
        expr = expr.mid(1, expr.size() - 2);
    }

    // ARM style: [x16, 0xb48] → x16+0xb48
    expr.replace(",", "+");

    // remove ARM immediates
    expr.replace("#", "");

    // ARM shift syntax: lsl → <<
    QRegularExpression lslRegex(R"(\\b([a-zA-Z0-9]+)\\s*\\+?\\s*([a-zA-Z0-9]+)\\s*lsl\\s*(\\d+))");
    expr.replace(QRegularExpression("lsl"), "<<");

    // MIPS/RISCV style: 0x10(sp) → sp+0x10
    QRegularExpression baseOffset(R"((0x[0-9a-fA-F]+|\d+)\((\w+)\))");
    expr.replace(baseOffset, "\\2+\\1");

    // remove extra spaces
    expr.replace(QRegularExpression("\\s+"), "");
    expr.replace(QChar(0xA0), ' '); // remove non-breaking spaces
    expr.replace(" ", "");          // optional: remove spaces entirely
    return expr;
}

DisassemblyHelper::Token DisassemblyHelper::getToken(QTextCursor cursor){
    cursor.select(QTextCursor::WordUnderCursor);
    DisassemblyHelper::Token token;
    token.ref = false;
    token.type = DisassemblyHelper::TokenType::Undef;
    token.isStack = false;

    QString line = cursor.block().text();

    int blockPos = cursor.block().position();

    int start = cursor.selectionStart() - blockPos;
    int end   = cursor.selectionEnd()   - blockPos;

    bool started=false,ended=false;
    int tokenStart =start,tokenEnd=end;

    bool inBracket = false;

    QString word = cursor.selectedText();

    // scan left
    token.token = word;
    while (start > 0) {
        QChar c = line[start - 1];
        if(!started&&!(c.isLetterOrNumber() || c == '.' || c == '_')){
            started = true;
            tokenStart = start;
        }
        if (c == ';')
            break;

        if (c == '[') {
            inBracket = true;
            start--;
            break;
        }
        start--;
    }

    bool foundClose = false;
    // scan right
    while (end < line.size()) {
        QChar c = line[end];
        if(!ended&&!(c.isLetterOrNumber() || c == '.' || c == '_')){
            ended = true;
            tokenEnd = end;
        }
        if (c == ';')
            break;

        if (c == ']') {
            end++;
            foundClose = true;
            break;
        }
        end++;
    }

    if (foundClose&&inBracket) {
        token.ref = true;
        token.expression = line.mid(start, end - start).trimmed();
        token.exparg = "";
    };

    token.token = line.mid(tokenStart,tokenEnd-tokenStart).trimmed();
    auto lock = Core()->lock();
    RVA rva = DisassemblyHelper::readDisassemblyOffset(cursor);
    token.offset = rva;

    //varcheck
    QList<VariableDescription> vardesc = Core()->getVariables(rva);
    for(auto v: vardesc){
        if(v.name == token.token){
            token.type = TokenType::Variable;
            token.vardesc = v;
            if(token.ref&&v.value.startsWith("0x")){
                token.expression.replace(v.name,v.value.split(QRegularExpression("[^A-Za-z0-9]+"))[0]);
            }
        }
    }

    //regcheck
    RzReg *reg = Core()->getReg();
    RzRegItem *item = rz_reg_get(reg, token.token.toUtf8().constData(), RZ_REG_TYPE_ANY);

    if (item&&item->name) {
        token.points = rz_reg_get_value(reg, item);
        token.type = DisassemblyHelper::TokenType::Register;
        token.regitem = item;
    }

    //stackcheck
    RzRegItem *sp = rz_reg_get(reg, "SP", RZ_REG_TYPE_ANY);

    if (QString(sp->name)==token.token&&!token.expression.isEmpty()) {
        ut64 spValue = rz_reg_get_value(reg, sp);

        int ptrSize = lock->analysis->bits / 8;

        QByteArray bytes(ptrSize * 5, 0);
        QString info="";
        rz_io_pread_at(
            lock->io,
                       spValue,
                       reinterpret_cast<ut8*>(bytes.data()),
                       bytes.size()
        );
        for (int i = 0; i < 5; i++) {
            ut64 value = 0;
            memcpy(&value, bytes.data() + i * ptrSize, ptrSize);

            info += QString("[%1] 0x%2<br>").arg(i).arg(value,0,16);
        }
        token.stackValues = info;
        token.isStack = true;
        return token;
    }

    //flagcheck
    if (token.type == DisassemblyHelper::TokenType::Undef){
        RzFlagItem *flag = rz_flag_get(lock->flags, token.token.toUtf8().constData());
        if(flag){
            token.type = TokenType::Symbol;
            ut64 addr = flag->offset;
            token.points = addr;
            return token;
        }
    }

    //immediate check

    if(token.type == TokenType::Undef&&token.token.startsWith("0x")){
        token.type = TokenType::Immediate;
        token.value = QString(token.token).remove("0x").toULongLong(nullptr, 16);
        return token;
    }

    //symbol check
    auto symbols = Core()->getAllSymbols();
    for(auto &a:symbols){
        if(a.name==token.token){
            token.type = TokenType::Symbol;
            token.points = a.vaddr;
            return token;
        }
    }

    return token;
}


