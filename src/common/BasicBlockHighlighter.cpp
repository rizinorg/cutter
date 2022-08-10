#include "BasicBlockHighlighter.h"

BasicBlockHighlighter::BasicBlockHighlighter() {}

/**
 * @brief Highlight the basic block at address
 */
void BasicBlockHighlighter::highlight(RVA address, const QColor &color)
{
    BasicBlock block;
    block.address = address;
    block.color = color;
    bbMap[address] = block;
}

/**
 * @brief Clear the basic block highlighting
 */
void BasicBlockHighlighter::clear(RVA address)
{
    bbMap.erase(address);
}

/**
 * @brief Return a highlighted basic block
 *
 * If there is nothing to highlight at specified address, returns nullptr
 */
BasicBlockHighlighter::BasicBlock *BasicBlockHighlighter::getBasicBlock(RVA address)
{
    auto it = bbMap.find(address);
    if (it != bbMap.end()) {
        return &it->second;
    }

    return nullptr;
}
