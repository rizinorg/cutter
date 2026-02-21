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

DisassemblyTextBlockUserData *getUserData(const QTextBlock &block);

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

}

#endif // DISASSEMBLYHELPER_H
