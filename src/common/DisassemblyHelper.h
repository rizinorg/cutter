#ifndef DISASSEMBLYHELPER_H
#define DISASSEMBLYHELPER_H

#include <QTextBlockUserData>
#include "core/CutterDescriptions.h"

/**
 * @brief Metadata container attached to each QTextBlock in the disassembly view
 *
 * Each visible line in disassembly related widgets is treated as a single text block
 */
class DisassemblyTextBlockUserData : public QTextBlockUserData
{
public:
    DisassemblyLine line;

    explicit DisassemblyTextBlockUserData(const DisassemblyLine &line);
};

/**
 * @brief Helpers for translating UI elements (words, lines) into disassembly data
 */
namespace DisassemblyHelper {

/**
 * @brief Identifies what kind of item was clicked or hovered
 */
enum class TargetType {
    VariableXRef,
    VariableValue,
    TypeName,
    XRefComment,
    Arrow,
    Register,
    Memory,
    MMIO,
    None,
};

/**
 * @brief Data used to figure out what is under the mouse
 */
struct TargetContext
{
    RVA offset;
    RVA arrow;
    QString word;
    QString line;
};

/**
 * @brief What was found after checking the context
 */
struct TargetAction
{
    RVA value;
    TargetType type;
};

/**
 * @brief Filter to control how deep the search goes
 */
enum TargetFilter {
    XRefComments = 1 << 0,
    VariableXrefs = 1 << 1,
    VariableValues = 1 << 2,
    Types = 1 << 3,
    Arrows = 1 << 4,
    Registers = 1 << 5,
    Memory = 1 << 6,
    MMIO = 1 << 7,

    Standard = XRefComments | VariableXrefs | Types | Arrows,
    Debug = Registers | Memory | MMIO | VariableValues,
    All = Standard | Debug
};

/**
 * @brief Result of bracket detection
 */
struct BracketResult
{
    bool found = false;
    int start = -1;
    int length = 0;
    QString content;
};

DisassemblyTextBlockUserData *getUserData(const QTextBlock &block);

/**
 * @brief Finds the range and content of a bracketed expression under a given position
 * @param line The text line to search in
 * @param posInLine The cursor position within the line
 * @return BracketResult containing the found range and content
 */
BracketResult findBracketRange(const QString &line, int posInLine);

/**
 * @brief Finds the source (from) address of an XRef based on the text word under the cursor
 * @param offset The base offset of the line which contains an XREF to it
 * @param selectedWord The specific text string being hovered (must be an address)
 * @return The source RVA of the XRef, or RVA_INVALID if not found
 */
RVA getXRefFromWord(RVA offset, const QString &selectedWord);

/**
 * @brief Checks if a disassembly line is an auto-generated XRef metadata line
 * @param offset The offset of the current disassembly line/block
 * @param line The full text content of the disassembly line/block
 * @return True if the line is an analysis-generated XRef comment
 */
bool isXRefFromComment(RVA offset, const QString &line);

/*!
 * @brief Reads the offset for the cursor position
 * @return The disassembly offset of the hovered asm text
 */
RVA readDisassemblyOffset(QTextCursor tc);

/*!
 * @brief Reads the arrow offset for the cursor position
 * @return Offset the arrow points to
 */
RVA readDisassemblyArrow(QTextCursor tc);

/**
 * @brief Gets the text and address at the current cursor position
 * @param tc The cursor to check
 * @return TargetContext containing info about the offset, word and line at the cursor
 */
TargetContext getContextFromCursor(QTextCursor tc);

/**
 * @brief Identifies what a context points to (like a variable or type)
 * @param ctx The context found at the cursor
 * @param filter Limits what kind of items to look for
 * @return The result containing the address and type found
 */
TargetAction resolveTarget(const TargetContext &ctx, int filter = TargetFilter::All);
}

#endif // DISASSEMBLYHELPER_H
