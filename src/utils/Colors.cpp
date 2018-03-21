#include "Colors.h"
#include "utils/Configuration.h"

Colors::Colors()
{

}


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
// Copied from R_API const char* r_print_color_op_type(RPrint *p, ut64 anal_type) {
QString Colors::getColor(ut64 type)
{
    switch (type & R_ANAL_OP_TYPE_MASK) {
    case R_ANAL_OP_TYPE_NOP:
        return "nop";
    case R_ANAL_OP_TYPE_ADD:
    case R_ANAL_OP_TYPE_SUB:
    case R_ANAL_OP_TYPE_MUL:
    case R_ANAL_OP_TYPE_DIV:
    case R_ANAL_OP_TYPE_MOD:
    case R_ANAL_OP_TYPE_LENGTH:
        return "math";
    case R_ANAL_OP_TYPE_AND:
    case R_ANAL_OP_TYPE_OR:
    case R_ANAL_OP_TYPE_XOR:
    case R_ANAL_OP_TYPE_NOT:
    case R_ANAL_OP_TYPE_SHL:
    case R_ANAL_OP_TYPE_SAL:
    case R_ANAL_OP_TYPE_SAR:
    case R_ANAL_OP_TYPE_SHR:
    case R_ANAL_OP_TYPE_ROL:
    case R_ANAL_OP_TYPE_ROR:
    case R_ANAL_OP_TYPE_CPL:
        return "bin";
    case R_ANAL_OP_TYPE_IO:
        return "swi";
    case R_ANAL_OP_TYPE_JMP:
    case R_ANAL_OP_TYPE_UJMP:
    case R_ANAL_OP_TYPE_IJMP:
    case R_ANAL_OP_TYPE_RJMP:
    case R_ANAL_OP_TYPE_IRJMP:
    case R_ANAL_OP_TYPE_MJMP:
        return "jmp";
    case R_ANAL_OP_TYPE_CJMP:
    case R_ANAL_OP_TYPE_UCJMP:
    case R_ANAL_OP_TYPE_SWITCH:
        return "cjmp";
    case R_ANAL_OP_TYPE_CMP:
    case R_ANAL_OP_TYPE_ACMP:
        return "cmp";
    case R_ANAL_OP_TYPE_UCALL:
    case R_ANAL_OP_TYPE_ICALL:
    case R_ANAL_OP_TYPE_RCALL:
    case R_ANAL_OP_TYPE_IRCALL:
    case R_ANAL_OP_TYPE_UCCALL:
    case R_ANAL_OP_TYPE_CALL:
    case R_ANAL_OP_TYPE_CCALL:
        return "call";
    case R_ANAL_OP_TYPE_NEW:
    case R_ANAL_OP_TYPE_SWI:
        return "swi";
    case R_ANAL_OP_TYPE_ILL:
    case R_ANAL_OP_TYPE_TRAP:
        return "trap";
    case R_ANAL_OP_TYPE_CRET:
    case R_ANAL_OP_TYPE_RET:
        return "ret";
    case R_ANAL_OP_TYPE_CAST:
    case R_ANAL_OP_TYPE_MOV:
    case R_ANAL_OP_TYPE_LEA:
    case R_ANAL_OP_TYPE_CMOV: // TODO: add cmov cathegory?
        return "mov";
    case R_ANAL_OP_TYPE_PUSH:
    case R_ANAL_OP_TYPE_UPUSH:
    case R_ANAL_OP_TYPE_LOAD:
        return "push";
    case R_ANAL_OP_TYPE_POP:
    case R_ANAL_OP_TYPE_STORE:
        return "pop";
    case R_ANAL_OP_TYPE_CRYPTO:
        return "crypto";
    case R_ANAL_OP_TYPE_NULL:
        return "other";
    case R_ANAL_OP_TYPE_UNK:
    default:
        return "invalid";
    }
}

