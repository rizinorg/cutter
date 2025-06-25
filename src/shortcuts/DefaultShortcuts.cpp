#include "DefaultShortcuts.h"

const QHash<QString, Shortcut> &getDefaultShortcuts()
{
    static const QHash<QString, Shortcut> defaultShortcuts = {

        // General
        { "focusConsole", { { Qt::Key_Period }, "Focus Console Input" } },
        { "gotoEntry", { { Qt::Key_G }, "Go to Entry" } },
        { "seek", { { Qt::Key_S }, "Seek" } },
        { "seekToFunctionEnd", { { Qt::Key_Dollar }, "Seek to Function End" } },
        { "seekToFunctionStart", { { Qt::Key_AsciiCircum }, "Seek to Function Start" } },
        { "refreshContens", { { Qt::CTRL | Qt::Key_R }, "Refresh Contents" } },
        { "back", { { QKeySequence::Back }, "Undo Seek" } },
        { "forward", { { QKeySequence::Forward }, "Redo Seek" } },
        { "zoomIn", { { Qt::Key_Plus }, "Zoom In" } },
        { "zoomOut", { { Qt::Key_Minus }, "Zoom Out" } },

        // AddressableItem
        { "AddressableItem.copyAddress", { { Qt::CTRL | Qt::SHIFT | Qt::Key_C }, "Copy address" } },
        { "AddressableItem.showXRefs", { { Qt::Key_X }, "Show X-Refs" } },
        { "AddressableItem.addComment", { { Qt::Key_Semicolon }, "Add Comment" } },
        { "AddressableItem.toggleBreakpoint", { { Qt::Key_F2 }, "Add Breakpoint" } },

        // Breakpoint
        { "Breakpoint.delBreakpoint", { { Qt::Key_Delete }, "Delete Breakpoint" } },
        { "Breakpoint.toggleBreakpoint", { { Qt::Key_Delete }, "Add/Remove Breakpoint" } },

        // Console
        { "Console.toggle",
          { { Qt::CTRL | Qt::Key_QuoteLeft, Qt::Key_Colon }, "Toggle Console Window" } },
        { "Console.clear", { { Qt::CTRL | Qt::Key_L }, "Clear Output" } },
        { "Console.clearRzInputLineEdit", { { Qt::Key_Escape }, "Clear Input" } },
        { "Console.clearDebugee", { { Qt::Key_Escape }, "Clear Debugee" } },
        { "Console.historyUp", { { Qt::Key_Up }, "Previous Command" } },
        { "Console.historyDown", { { Qt::Key_Down }, "Next Command" } },
        { "Console.complete", { { Qt::Key_Tab }, "Auto-Complete" } },

        // Debug
        { "Debug.start", { { Qt::Key_F9 }, "Start Debug" } },
        { "Debug.continue", { { Qt::Key_F5 }, "Continue" } },
        { "Debug.continueBack", { { Qt::CTRL | Qt::Key_F5 }, "Continue Backwards" } },
        { "Debug.step", { { Qt::Key_F7 }, "Step" } },
        { "Debug.stepOver", { { Qt::Key_F8 }, "Step Over" } },
        { "Debug.stepOut", { { Qt::CTRL | Qt::Key_F8 }, "Step Out" } },
        { "Debug.stepBack", { { Qt::CTRL | Qt::Key_F7 }, "Step Backwards" } },
        { "Debug.accept", { { QKeySequence(Qt::CTRL | Qt::Key_Return) }, "Accept Dialog" } },

        // Decompiler
        { "Decompiler.copy", { { QKeySequence::Copy }, "Copy" } },
        { "Decompiler.copyReferenceAddress",
          { { Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier
              | Qt::Key_C },
            "Copy Reference Address" } },
        { "Decompiler.addComment", { { Qt::Key_Semicolon }, "Add Comment" } },
        { "Decompiler.showXRefs", { { Qt::Key_X }, "Show X-Refs" } },
        { "Decompiler.renameThingHere",
          { { Qt::Key_N }, "Rename Local Variables Defined in Disassembly" } },
        { "Decompiler.editFunctionVariables",
          { { Qt::Key_Y }, "Edit Function Variables Defined in Disassembly" } },
        { "Decompiler.toggleBreakpoint",
          { { Qt::Key_F2, Qt::CTRL | Qt::Key_B }, "Add/Remove Breakpoint" } },
        { "Decompiler.advancedBreakpoint", { { Qt::CTRL | Qt::Key_F2 }, "Advanced Breakpoint" } },
        { "Decompiler.seekPrev", { { Qt::Key_Escape }, "Seek to Previous Address" } },

        // Disassembly
        { "Disassembly.copy", { { QKeySequence::Copy }, "Copy" } },
        { "Disassembly.copyAddress", { { Qt::CTRL | Qt::SHIFT | Qt::Key_C }, "Copy Address" } },
        { "Disassembly.copyInstructionBytes",
          { { Qt::CTRL | Qt::ALT | Qt::Key_C }, "Copy Instruction Bytes" } },
        { "Disassembly.addComment", { { Qt::Key_Semicolon }, "Add Comment" } },
        { "Disassembly.retypeLocals", { { Qt::Key_Y }, "Re-type Local Variables" } },
        { "Disassembly.editFunction", { { Qt::SHIFT | Qt::Key_P }, "Edit Function" } },
        { "Disassembly.undefineFunction", { { Qt::Key_U }, "Undefine Function" } },
        { "Disassembly.defineFunction", { { Qt::Key_P }, "Define Function Here" } },
        { "Disassembly.showXRefs", { { Qt::Key_X }, "Show X-Refs" } },
        { "Disassembly.XRefsForVariables",
          { { QKeySequence(Qt::SHIFT | Qt::Key_X) }, "X-Refs for Local Variables" } },
        { "Disassembly.showOptions", { { Qt::Key_D }, "Show Options" } },
        { "Disassembly.rename", { { Qt::Key_N }, "Rename or Add Flag" } },
        { "Disassembly.globalVariable", { { Qt::Key_G }, "Modify or Add Global Variable" } },
        { "Disassembly.setToCode", { { Qt::Key_C }, "Code" } },
        { "Disassembly.setAsString", { { Qt::Key_A }, "Auto-detect String" } },
        { "Disassembly.setAsStringAdvanced", { { Qt::SHIFT | Qt::Key_A }, "Advanced String" } },
        { "Disassembly.setToDataEx", { { Qt::Key_Asterisk }, "..." } },
        { "Disassembly.setToData", { { Qt::Key_D }, "Switch Data" } },
        { "Disassembly.toggleBreakpoint",
          { { Qt::Key_F2, Qt::CTRL | Qt::Key_B }, "Add/Remove Breakpoint" } },
        { "Disassembly.advancedBreakpoint", { { Qt::CTRL | Qt::Key_F2 }, "Advanced Breakpoint" } },
        { "Disassembly.switchToGraph", { { Qt::Key_Space }, "Switch to Graph" } },
        { "Disassembly.seekPrev", { { Qt::Key_Escape }, "Seek to Previous Address" } },
        { "Disassembly.moveDownJ", { { Qt::Key_J }, "Move Cursor Down" } },
        { "Disassembly.moveDown", { { QKeySequence::MoveToNextLine }, "Move Cursor Down" } },
        { "Disassembly.moveUpK", { { Qt::Key_K }, "Move Cursor Up" } },
        { "Disassembly.moveUp", { { QKeySequence::MoveToPreviousLine }, "Move Cursor Up" } },
        { "Disassembly.pageDown",
          { { QKeySequence::MoveToNextPage }, "Move Cursor Down By Page" } },
        { "Disassembly.pageUp",
          { { QKeySequence::MoveToPreviousPage }, "Move Cursor Up By Page" } },

        // Exports
        { "Exports.toggle", { { Qt::SHIFT | Qt::Key_E }, "Toggle Exports Window" } },

        // Flags
        { "Flags.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "Flags.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // Functions
        { "Functions.rename", { { Qt::Key_N }, "Rename Function" } },

        // Globals
        { "Globals.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "Globals.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // Graph
        { "Graph.seekPrev", { { Qt::Key_Escape }, "Seek to Previous Address" } },
        { "Graph.takeTrue", { { Qt::Key_T }, "Take True Branch" } },
        { "Graph.takeFalse", { { Qt::Key_F }, "Take False Branch" } },
        { "Graph.nextInstr", { { Qt::Key_J }, "Next Instruction" } },
        { "Graph.prevInstr", { { Qt::Key_K }, "Previous Instruction" } },
        { "Graph.toggle", { { Qt::SHIFT | Qt::Key_G }, "Toggle Graph Window" } },
        { "Graph.switchToDisassembly", { { Qt::Key_Space }, "Switch to Disassembly View" } },

        // Hex
        { "Hex.copy", { { QKeySequence::Copy }, "Copy Selection" } },
        { "Hex.copyAddress", { { Qt::CTRL | Qt::SHIFT | Qt::Key_C }, "Copy Address" } },
        { "Hex.addComment", { { Qt::Key_Semicolon }, "Add Comment" } },
        { "Hex.addFlag", { { Qt::Key_N }, "Add Flag at Address" } },

        // Imports
        { "Imports.toggle", { { Qt::SHIFT | Qt::Key_I }, "Toggle Imports Window" } },

        // ListDock
        { "ListDock.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "ListDock.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // Omnibar
        { "Omnibar.clear", { { QKeySequence(Qt::Key_Escape) }, "Clear Omnibar" } },

        // Processes
        { "Processes.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "Processes.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // RegisterRefs
        { "RegisterRefs.showFilter", { { QKeySequence::Find }, "Show Filter" } },

        // Strings
        { "Strings.toggle", { { Qt::SHIFT | Qt::Key_F12 }, "Toggle Strings Window" } },
        { "Strings.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "Strings.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // Threads
        { "Threads.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "Threads.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // Types
        { "Types.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "Types.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },

        // VTables
        { "VTables.showFilter", { { QKeySequence::Find }, "Show Filter" } },
        { "VTables.clearFilter", { { Qt::Key_Escape }, "Clear Filter" } },
    };
    return defaultShortcuts;
}
