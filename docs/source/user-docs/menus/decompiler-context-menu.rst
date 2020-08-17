Decompiler Context Menu 
==============================

.. toctree::
   :maxdepth: 1
   :glob:

   decompiler-context-menu/*

The decompiler context menu contains actions that are available for the location under cursor.

Copy
----------------------------------------
**Description:** If text is selected, copy selected text. Otherwise, if a word is highlighted, copy that word. In all other cases, copy the line under the cursor.

**Steps:**  Right-click on a selected text and choose ``Copy``  

**Shortcut:** :kbd:`Ctrl` + :kbd:`C`  

Copy Instruction Address
----------------------------------------
**Description:** Copy the instruction address mapped to the part of code under the cursor.

**Steps:**  Right-click on a location and choose ``Copy instruction address(<address>)``

Copy Address of Reference
----------------------------------------
**Description:** Copy the address of the reference under cursor. References include functions, global variables, and constant variables with an address.

**Steps:**  Right-click on a location and choose ``Copy  address of [<name of reference>](<address>)``  

**Shortcut:** :kbd:`Ctrl` + :kbd:`Shift` + :kbd:`C`

Show the code in another widget
----------------------------------------
**Description:** Show the code under cursor in another opened widget, or open a new one. If a non-decompiler widget is chosen, the address mapped to the portion of code under the cursor will be opened in that widget.

**Steps:**  Right-click on an item and choose the ``Show in`` sub-menu.

Show the selected item in another widget
----------------------------------------
**Description:** Show the selected item in another opened widget, or open a new one. Items include functions, global variables, and constant variables under the cursor.

**Steps:**  Right-click on an item and choose the ``Show [item] in`` sub-menu.

Add and Edit Comment
----------------------------------------
**Description:** Add a comment in the location under the cursor or edit the comment in the location under the cursor. Edit comment option is available only for user defined comments.

**Steps:** Right-click and choose `Add Comment` or `Edit Comment`.

**Shortcut:** :kbd:`;`

Delete a Comment
----------------------------------------
**Description:** Delete the comment at the location under the cursor. This option is available only for user defined comments.

**Steps:** Right-click on an instruction with a user-defined comment and choose ``Delete comment``  

Rename function
----------------------------------------
**Description:** Rename the function under cursor. 

**Steps:** Right-click the function name and choose ``Rename function <name of function>``  

**Shortcut:** :kbd:`N`

Give a name or rename global variables
----------------------------------------
**Description:** Give a name or rename the global variable under the cursor.

**Steps:** Right-click the global variable and choose ``Add name to <address of global variable>`` or ``Rename [name]``.

**Shortcut:** :kbd:`N`

Delete the name of global variable
----------------------------------------
**Description:** Delete the name of the global variable under the cursor.

**Steps:** Right-click the global variable and choose ``Remove [name of global variable]``.

Rename Function Variable
----------------------------------------
**Description:** Rename local variables and arguments in the decompiled function. Note that this option is available only for function variables defined in the disassembly.

**Steps:** Right-click the variable and choose ``Rename variable [name]``. 

**Shortcut:** :kbd:`N` 

Edit Local Variables and Arguments
----------------------------------------
**Description:** Rename or set the types of the function's variables and arguments. Note that this option is available only for function variables defined in the disassembly.

**Steps:** Right-click on the variable and choose ``Edit variable [name]``.

**Shortcut:** :kbd:`Y`

Show Cross References
----------------------------------------
**Description:** Show X-Refs to the reference under cursor. This option will open Cutter's X-Refs dialog in which you will be able to see a list of X-Refs from and to the address of the reference. You can also see a preview of each cross-reference to quickly inspect the different usages. Note that references refer to function names, global variables, and constant variables with an address.  

**Steps:** Right-click on a reference and choose ``Show X-Refs``  

**Shortcut:** :kbd:`X`
