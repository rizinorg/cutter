Decompiler Context Menu 
==============================
The decompiler context menu is a context-sensitive menu that contains actions that are available for the position under the cursor.

Copy
----------------------------------------
**Description:** If text is selected, copy the selected text to the clipboard. If a word is highlighted, copy that word. In all other cases, copy the line under the cursor.

**Steps:**  Right-click on a selected text and choose ``Copy``

**Shortcut:** :kbd:`Ctrl-C`  

Copy Instruction Address
----------------------------------------
**Description:** Copy the address of the instruction mapped to the part of the code under the cursor.

**Steps:**  Right-click on the portion of code for which you want the instruction's address and choose ``Copy instruction address (<address>)``

Copy Address of Reference
----------------------------------------
**Description:** Copy the address of the reference under the cursor. References include functions, global variables, and constant variables with an address.

**Steps:**  Right-click on a reference and choose ``Copy  address [of <name>] (<address>)``  

**Shortcut:** :kbd:`Ctrl-Shift-C`

Show the code in another widget
----------------------------------------
**Description:** Show the code under the cursor in another opened widget, or open a new one. If a non-decompiler widget is chosen, the address mapped to the portion of code under the cursor will be opened in that widget.

**Steps:**  Right-click on an item and go to the :menuselection:`Show in` submenu. You can choose a widget or open a new widget from here.

Show the selected item in another widget
----------------------------------------
**Description:** Show the selected item in another opened widget, or open a new one. Items include functions, global variables, and constant variables under the cursor.

**Steps:**  Right-click on an item and go to the submenu :menuselection:`Show <item> in` or :menuselection:`<function name> (<address>)`. You can choose a widget or open a new widget from here.

Add and Edit Comment
----------------------------------------
**Description:** Add a comment for the line of code under the cursor or edit the comment under the cursor. The ``Edit comment`` option is only available for user-defined comments.

**Steps:** Right-click and choose ``Add Comment`` or ``Edit Comment``.

**Shortcut:** :kbd:`;`

Delete a Comment
----------------------------------------
**Description:** Delete the comment under the cursor. If a comment doesn't exist under the cursor, delete the comment at the offset mapped to the portion of code under the cursor.

**Steps:** Right-click on an instruction with a user-defined comment and choose ``Delete comment``  

Rename function
----------------------------------------
**Description:** Rename a function under the cursor. 

**Steps:** Right-click on a function name and choose ``Rename function <name>``  

**Shortcut:** :kbd:`N`

Give a name or rename global variables
----------------------------------------
**Description:** Give a name or rename the global variable under the cursor.

**Steps:** Right-click the global variable and choose ``Add name to <address of global variable>`` or ``Rename <name>``.

**Shortcut:** :kbd:`N`

Delete the name of a global variable
----------------------------------------
**Description:** Delete the name of the global variable under the cursor.

**Steps:** Right-click on a global variable and choose ``Remove <name>``.

Rename Function Variable
----------------------------------------
**Description:** Rename local variables and arguments in the decompiled function. Note that this option is available only for function variables defined in the disassembly.

**Steps:** Right-click on a variable and choose ``Rename variable <name>``. 

**Shortcut:** :kbd:`N` 

Edit Local Variables and Arguments
----------------------------------------
**Description:** Rename or set the types of the function's variables and arguments. Note that this option is available only for function variables and arguments defined in the disassembly.

**Steps:** Right-click on a variable and choose ``Edit variable <name>``.

**Shortcut:** :kbd:`Y`

Show Cross References
----------------------------------------
**Description:** Show X-Refs to the reference under the cursor. This option will open Cutter's X-Refs dialog in which you will be able to see a list of X-Refs from and to the address of the reference. You can also see a preview of each cross-reference to quickly inspect the different usages. Note that references refer to function names, global variables, and constant variables with an address.  

**Steps:** Right-click on a reference and choose ``Show X-Refs``  

**Shortcut:** :kbd:`X`


Debug Context Menu
=======================================

Add or Remove Breakpoint
-------------------------
**Description:** Add a breakpoint at the earliest offset in the line under the cursor. If you use the keyboard shortcut to remove a breakpoint, all the breakpoints defined in the line will be removed.

**Steps:** Right-click on a line of code and choose :menuselection:`Breakpoint --> Add breakpoint`  or :menuselection:`Breakpoint --> Remove breakpoint`.

**Shortcut:** :kbd:`F2`  

Advanced Breakpoint Dialog
----------------------------------------
**Description:** Open the advanced breakpoint dialog. This dialog lets you define not only a regular breakpoint in this address, but also a hardware breakpoint, a conditional breakpoint, and more.

**Steps:** Right-click on a line of code and choose :menuselection:`Breakpoint --> Advanced breakpoint`. If multiple breakpoints are present in the line, you will be able choose the breakpoint you want to edit from the :menuselection:`Edit breakpoint` submenu.

**Shortcut:** :kbd:`Ctrl-F2`

Continue Until Line
----------------------------------------
**Description:** Continue the execution of the program until it reaches the offset in the selected line. The program is not guaranteed to ever reach this address and will keep running until exited or until reached another breakpoint. If other breakpoints hit before reaching this line, they will be triggered and pause the execution. *This option is only available on Debug or Emulation modes*.      

**Steps:** While in Debug or Emulation modes, right-click on a line of code and choose :menuselection:`Debug --> Continue until line`.  

Set Program Counter (PC)
----------------------------------------
**Description:** Set the Program Counter of the debugger to the current offset. For example, on an Intel 64bit program, Cutter will set the value of the RIP register to the current address.  *This option is only available on Debug or Emulation modes*.  

**Steps:** While in Debug or Emulation modes, right-click on a line of code and choose :menuselection:`Debug --> Set PC`.
