#include "Colors.h"
#include "common/Configuration.h"

Colors::Colors() {}

void Colors::colorizeAssembly(RichTextPainter::List &list, QString opcode, ut64 type_num)
{
    RichTextPainter::CustomRichText_t assembly;
    assembly.highlight = false;
    assembly.flags = RichTextPainter::FlagColor;

    // TODO cut opcode and use op["ptr"] to colorate registers and immediate values
    assembly.text = opcode;

    QString colorName = Colors::getColor(type_num);
    assembly.textColor = ConfigColor(colorName);
    list.push_back(assembly);
}

// Temporary solution
// Copied from RZ_API const char* r_print_color_op_type(RPrint *p, ut64 analysis_type) {
QString Colors::getColor(ut64 type)
{
    switch (type & RZ_ANALYSIS_OP_TYPE_MASK) {
    case RZ_ANALYSIS_OP_TYPE_NOP:
        return "nop";
    case RZ_ANALYSIS_OP_TYPE_ADD:
    case RZ_ANALYSIS_OP_TYPE_SUB:
    case RZ_ANALYSIS_OP_TYPE_MUL:
    case RZ_ANALYSIS_OP_TYPE_DIV:
    case RZ_ANALYSIS_OP_TYPE_MOD:
    case RZ_ANALYSIS_OP_TYPE_LENGTH:
        return "math";
    case RZ_ANALYSIS_OP_TYPE_AND:
    case RZ_ANALYSIS_OP_TYPE_OR:
    case RZ_ANALYSIS_OP_TYPE_XOR:
    case RZ_ANALYSIS_OP_TYPE_NOT:
    case RZ_ANALYSIS_OP_TYPE_SHL:
    case RZ_ANALYSIS_OP_TYPE_SAL:
    case RZ_ANALYSIS_OP_TYPE_SAR:
    case RZ_ANALYSIS_OP_TYPE_SHR:
    case RZ_ANALYSIS_OP_TYPE_ROL:
    case RZ_ANALYSIS_OP_TYPE_ROR:
    case RZ_ANALYSIS_OP_TYPE_CPL:
        return "bin";
    case RZ_ANALYSIS_OP_TYPE_IO:
        return "swi";
    case RZ_ANALYSIS_OP_TYPE_JMP:
    case RZ_ANALYSIS_OP_TYPE_UJMP:
    case RZ_ANALYSIS_OP_TYPE_IJMP:
    case RZ_ANALYSIS_OP_TYPE_RJMP:
    case RZ_ANALYSIS_OP_TYPE_IRJMP:
    case RZ_ANALYSIS_OP_TYPE_MJMP:
        return "jmp";
    case RZ_ANALYSIS_OP_TYPE_CJMP:
    case RZ_ANALYSIS_OP_TYPE_UCJMP:
    case RZ_ANALYSIS_OP_TYPE_SWITCH:
        return "cjmp";
    case RZ_ANALYSIS_OP_TYPE_CMP:
    case RZ_ANALYSIS_OP_TYPE_ACMP:
        return "cmp";
    case RZ_ANALYSIS_OP_TYPE_UCALL:
    case RZ_ANALYSIS_OP_TYPE_ICALL:
    case RZ_ANALYSIS_OP_TYPE_RCALL:
    case RZ_ANALYSIS_OP_TYPE_IRCALL:
    case RZ_ANALYSIS_OP_TYPE_UCCALL:
    case RZ_ANALYSIS_OP_TYPE_CALL:
    case RZ_ANALYSIS_OP_TYPE_CCALL:
        return "call";
    case RZ_ANALYSIS_OP_TYPE_NEW:
    case RZ_ANALYSIS_OP_TYPE_SWI:
        return "swi";
    case RZ_ANALYSIS_OP_TYPE_ILL:
    case RZ_ANALYSIS_OP_TYPE_TRAP:
        return "trap";
    case RZ_ANALYSIS_OP_TYPE_CRET:
    case RZ_ANALYSIS_OP_TYPE_RET:
        return "ret";
    case RZ_ANALYSIS_OP_TYPE_CAST:
    case RZ_ANALYSIS_OP_TYPE_MOV:
    case RZ_ANALYSIS_OP_TYPE_LEA:
    case RZ_ANALYSIS_OP_TYPE_CMOV: // TODO: add cmov cathegory?
        return "mov";
    case RZ_ANALYSIS_OP_TYPE_PUSH:
    case RZ_ANALYSIS_OP_TYPE_UPUSH:
    case RZ_ANALYSIS_OP_TYPE_LOAD:
        return "push";
    case RZ_ANALYSIS_OP_TYPE_POP:
    case RZ_ANALYSIS_OP_TYPE_STORE:
        return "pop";
    case RZ_ANALYSIS_OP_TYPE_CRYPTO:
        return "crypto";
    case RZ_ANALYSIS_OP_TYPE_NULL:
        return "other";
    case RZ_ANALYSIS_OP_TYPE_UNK:
    default:
        return "invalid";
    }
}
