#include "BasicInstructionHighlighter.h"

BasicInstructionHighlighter::~BasicInstructionHighlighter() {
    for (BasicInstructionIt it = biMap.begin(); it != biMap.end(); ++it) {
        delete it->second;
    }
}

/**
 * @brief Clear the basic instruction highlighting
 */
void BasicInstructionHighlighter::clear(RVA address, RVA size) {
    BasicInstructionIt it = biMap.lower_bound(address);
    if (it != biMap.begin()) {
        --it;
    }

    std::vector<RVA> addrs;
    while (it != biMap.end() && it->first < address + size) {
        addrs.push_back(it->first);
        ++it;
    }

    // first and last entries may intersect, but not necessarily
    // be contained in [address, address + size), so we need to
    // check it and perhaps adjust their addresses.
    std::vector<BasicInstruction*> newInstructions;
    if (!addrs.empty()) {
        BasicInstruction *prev = biMap[addrs.front()];
        if (prev->address < address && prev->address + prev->size > address) {
            BasicInstruction *newInstr = new BasicInstruction;
            newInstr->address = prev->address;
            newInstr->size = address - prev->address;
            newInstr->color = prev->color;
            newInstructions.push_back(newInstr);
        }

        BasicInstruction *next = biMap[addrs.back()];
        if (next->address < address + size && next->address + next->size > address + size) {
            const RVA offset = address + size - next->address;
            BasicInstruction *newInstr = new BasicInstruction;
            newInstr->address = next->address + offset;
            newInstr->size = next->size - offset;
            newInstr->color = next->color;
            newInstructions.push_back(newInstr);
        }
    }

    for (RVA addr : addrs) {
        BasicInstruction *bi = biMap[addr];
        if (std::max(bi->address, address) < std::min(bi->address + bi->size, address + size)) {
            biMap.erase(addr);
            delete bi;
        }
    }

    for (BasicInstruction *newInstr : newInstructions) {
        biMap[newInstr->address] = newInstr;
    }
}

/**
 * @brief Highlight the basic instruction at address
 */
void BasicInstructionHighlighter::highlight(RVA address, RVA size, QColor color) {
    clear(address, size);
    BasicInstruction *bi = new BasicInstruction;
    bi->address = address;
    bi->size = size;
    bi->color = color;
    biMap[address] = bi;
}

/**
 * @brief Return a highlighted basic instruction
 *
 * If there is nothing to highlight at specified address, returns nullptr
 */
BasicInstruction* BasicInstructionHighlighter::getBasicInstruction(RVA address) {
    BasicInstructionIt it = biMap.upper_bound(address);
    if (it == biMap.begin()) {
        return nullptr;
    }

    BasicInstruction *bi = (--it)->second;
    if (bi->address <= address && address < bi->address + bi->size) {
        return bi;
    }
    return nullptr;
}