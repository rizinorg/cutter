#ifndef BASICKBLOCKHIGHLIGHTER_H
#define BASICKBLOCKHIGHLIGHTER_H

class BasicBlockHighlighter;

#include "Cutter.h"

#include <map>

/**
 * @brief Contains a highlight map for basic blocks
 */
class BasicBlockHighlighter
{
public:
    struct BasicBlock
    {
        RVA address;
        QColor color;
    };

    BasicBlockHighlighter();

    /**
     * @brief Highlight the basic block at address
     */
    void highlight(RVA address, const QColor &color);

    /**
     * @brief Clear the basic block highlighting
     */
    void clear(RVA address);

    /**
     * @brief Return a highlighted basic block
     * If there is nothing to highlight at specified address, returns nullptr
     */
    const BasicBlock *getBasicBlock(RVA address) const;

private:
    std::map<RVA, BasicBlock> bbMap;
};

#endif // BASICBLOCKHIGHLIGHTER_H
