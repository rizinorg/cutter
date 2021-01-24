#include "BasicBlockHighlighter.h"

BasicBlockHighlighter::BasicBlockHighlighter() {}

BasicBlockHighlighter::~BasicBlockHighlighter()
{
    for (BasicBlockIt itr = bbMap.begin(); itr != bbMap.end(); ++itr) {
        delete itr->second;
    }
}

/**
 * @brief Highlight the basic block at address
 */
void BasicBlockHighlighter::highlight(RVA address, const QColor &color)
{
    BasicBlock *block = new BasicBlock;
    block->address = address;
    block->color = color;
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
BasicBlock *BasicBlockHighlighter::getBasicBlock(RVA address)
{
    BasicBlockIt it;

    it = bbMap.find(address);
    if (it != bbMap.end()) {
        return it->second;
    }

    return nullptr;
}
