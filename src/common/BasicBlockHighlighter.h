#ifndef BASICKBLOCKHIGHLIGHTER_H
#define BASICKBLOCKHIGHLIGHTER_H

class BasicBlockHighlighter;

#include "Cutter.h"
#include <map>

struct BasicBlock {
    RVA address;
    QColor color;
};

typedef std::map<RVA, BasicBlock*>::iterator BasicBlockIt;

class BasicBlockHighlighter
{
public:
    BasicBlockHighlighter();
    ~BasicBlockHighlighter();

    void highlight(RVA address, const QColor &color);
    void clear(RVA address);
    BasicBlock *getBasicBlock(RVA address);

private:
    std::map<RVA, BasicBlock*> bbMap;
};

#endif   // BASICBLOCKHIGHLIGHTER_H
