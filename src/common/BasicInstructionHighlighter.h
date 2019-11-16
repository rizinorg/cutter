#ifndef BASICINSTRUCTIONHIGHLIGHTER_H
#define BASICINSTRUCTIONHIGHLIGHTER_H

class BasicInstructionHighlighter;

#include "Cutter.h"
#include <map> 

struct BasicInstruction {
    RVA address;
    RVA size;
    QColor color;
};

typedef std::map<RVA, BasicInstruction*>::iterator BasicInstructionIt;

class BasicInstructionHighlighter
{
public:
    ~BasicInstructionHighlighter();
    void clear(RVA address, RVA size);
    void highlight(RVA address, RVA size, QColor color);
    BasicInstruction* getBasicInstruction(RVA address);

private:
    std::map<RVA, BasicInstruction*> biMap;
};

#endif // BASICINSTRUCTIONHIGHLIGHTER_H
