#ifndef BASICKBLOCKHIGHLIGHTER_H
#define BASICKBLOCKHIGHLIGHTER_H

class BasicBlockHighlighter;

#include "Cutter.h"
#include <map>

class BasicBlockHighlighter
{
public:
    struct BasicBlock
    {
        RVA address;
        QColor color;
    };

    BasicBlockHighlighter();

    void highlight(RVA address, const QColor &color);
    void clear(RVA address);
    BasicBlock *getBasicBlock(RVA address);

private:
    std::map<RVA, BasicBlock> bbMap;
};

#endif // BASICBLOCKHIGHLIGHTER_H
