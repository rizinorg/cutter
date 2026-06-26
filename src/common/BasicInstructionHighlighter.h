#ifndef BASICINSTRUCTIONHIGHLIGHTER_H
#define BASICINSTRUCTIONHIGHLIGHTER_H

#include "CutterCommon.h"

#include <QColor>

#include <map>

/**
 * @brief Contains a highlight map for basic instructions
 */
class BasicInstructionHighlighter
{
public:
    struct BasicInstruction
    {
        RVA address;
        RVA size;
        QColor color;
    };

    typedef std::map<RVA, BasicInstruction>::iterator BasicInstructionIt;

    /**
     * @brief Clear the basic instruction highlighting
     */
    void clear(RVA address, RVA size);

    /**
     * @brief Highlight the basic instruction at address
     */
    void highlight(RVA address, RVA size, QColor color);

    /**
     * @brief Return a highlighted basic instruction
     * If there is nothing to highlight at specified address, returns nullptr
     */
    const BasicInstruction *getBasicInstruction(RVA address) const;

private:
    std::map<RVA, BasicInstruction> biMap;
};

#endif // BASICINSTRUCTIONHIGHLIGHTER_H
