Disassembly Context Menu 
==============================

.. toctree::
   :maxdepth: 1
   :glob:

   disassembly-context-menu/*

The Disassembly context menu contains actions that operate with selected instruction in disassembly and graph widgets.

Copy
----------------------------------------
**Description:** Copy the selected text.  

**Steps:**  Right-click on a selected text and choose ``Copy``  

**Shortcut:** :kbd:`Ctrl` + :kbd:`C`  

Copy Address
----------------------------------------
**Description:** Copy the address of the location under the cursor.  

**Steps:**  Right-click on a location and choose ``Copy address``  

**Shortcut:** :kbd:`Ctrl` + :kbd:`Shift` + :kbd:`C`  

Show address in another widget
----------------------------------------
**Description:** Show the selected address or item in another opened widget, or open a new one.  

**Steps:**  Right-click on an address or an item in instruction and choose the ``Show in`` sub-menu 

Add Comment
----------------------------------------
**Description:** Add a comment in the current location.  

**Steps:** Right-click an address and choose `Add Comment`.  

**Shortcut:** :kbd:`;`  

Add Flag
----------------------------------------
**Description:** Add a flag to the selected item or location by bookmarking and giving it a name.  

**Steps:** Right-click an address or item and choose ``Add Flag``. This will open the Flag dialog in which you can name the location.  

Rename
----------------------------------------
**Description:** Rename the flag, function or local variable at current location. If empty, remove the currently associated name.

**Steps:** Right-click an address or item and choose ``Rename``  

**Shortcut:** :kbd:`N`  

Edit Function
----------------------------------------
**Description:** Open the Function edit dialog in which you can define the name of the function, its start address, stack size, calling convention, and more.  

**Steps:**  Right-click on a location inside a function and choose ``Edit function``.  

**Shortcut:** :kbd:`Shift` + :kbd:`P`  

Re-type Local Variables
----------------------------------------
**Description:** Rename or set the types of the function's variables and arguments.  

**Steps:** Right-click anywhere inside a function and then choose ``Re-type Local Variables``.

**Shortcut:** :kbd:`Y`  

Delete a Comment
----------------------------------------
**Description:** Delete the comment at the current address. This option only available for addresses with user-defined comments. 

**Steps:** Right-click on an instruction with a user-defined comment and choose ``Delete comment``  

Delete a Flag
----------------------------------------
**Description:**   Delete the flag at the current location.

**Steps:** Right-click on a location with a flag and choose ``Delete flag``.  

Undefine a Function
----------------------------------------
**Description:** Undefine the current function. This will remove the function and its associated meta-data. You can always re-define the function, but every change that was made to the previously defined function (e.g variable renaming) would not be restored.  

**Steps:**  Right-Click on the name of the function and choose ``Undefine function``.  

**Shortcut:** :kbd:`U`  

Define a function
----------------------------------------
**Description:** Define a function starting from the current location. Cutter will automatically guess the size of the function. This can later be changed using the function editor.  

**Steps:** Right-click on an instruction and choose ``Define function here``.  

**Shortcut:** :kbd:`P`  

Set Structure Offset
----------------------------------------
**Description:** Present the current value is an offset in a structure. 

**Steps:**  -> Structure offset  

Link a Type to Address
----------------------------------------
**Description:** You can link type, enum or structure to a specific address. Types, structures and enums can be defined in the Types widget.  

**Steps:** Right-click on an instruction and choose ``Link Type to Address``.  

**Shortcut:** :kbd:`L`  


Show Cross References
----------------------------------------
**Description:** Show X-Refs from and to the specific location. This option will open Cutter's X-Refs dialog in which you will be able to see a list of X-Refs from and to the selected location, in addition to a preview of each cross-reference to quickly inspect the different usages.  

**Steps:** Right-click on an instruction and choose ``Show X-Refs``  

**Shortcut:** :kbd:`X`

