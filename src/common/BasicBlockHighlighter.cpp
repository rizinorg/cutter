#include "BasicBlockHighlighter.h"

BasicBlockHighlighter::BasicBlockHighlighter() {}

void BasicBlockHighlighter::highlight(RVA address, const QColor &color)
{
    BasicBlock block;
    block.address = address;
    block.color = color;
    bbMap[address] = block;
}

void BasicBlockHighlighter::clear(RVA address)
{
    bbMap.erase(address);
}

const BasicBlockHighlighter::BasicBlock *BasicBlockHighlighter::getBasicBlock(RVA address) const
{
    const auto it = bbMap.find(address);
    if (it != bbMap.end()) {
        return &it->second;
    }

    return nullptr;
}
