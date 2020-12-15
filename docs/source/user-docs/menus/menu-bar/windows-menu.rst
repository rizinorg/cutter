Windows Menu
==============================

Show Dashboard
----------------------------------------
**Description:** Show the Dashboard panel. Cutter's dashboard contains basic information about the binary. On the Dashboard you can find:
 - File name
 - Binary format (PE, ELF64, ...)  
 - Bits (16, 32, 64, ...)
 - Binary Architecture (x86, ARM, ...)
 - Access mode (Read, Write, Execute)
 - Size
 - Binary type
 - Written Language
 - Compiler detection
 - Stack Canaries
 - NX bit
 - Position independent code
 - Checksums (MD5, SHA1, ...)
 - Entropy
 - and more...


**Steps:** Windows -> Dashboard  

Show Functions
----------------------------------------
**Description:** Display the list of functions identified by Cutter. The list also contains information about each function such as name, address, size, and more. Some functions like Main, Entrypoint and external functions are highlighted with specific colors to make them easier to spot.   

**Steps:** Windows -> Functions  

Show Decompiler
----------------------------------------
**Description:** Cutter releases are shipped with two decompilers by default - [rz-ghidra](https://github.com/rizinorg/rz-ghidra) and [rz-dec](#) which will be available soon. The Decompiler view will display the decompilation of the current function. The widget is interactive and support address-syncing, renaming, re-typing and more. Cutter can be extended with more decompilers.   

**Steps:** Windows -> Decompiler  

Graph Overview
----------------------------------------
**Description:** One of the main views of Cutter allows you to navigate inside functions in a graph mode. Graph overview will only display a zoomed-out form of the graph, and will help the user understand the flow of a function as a whole.  

**Steps:** Windows -> Graph Overview  

Show Search
----------------------------------------
**Description:** Show the Search panel in which you can search data, strings, hex and more in the opened binary.  

**Steps:** Windows -> Search  

Show Strings
----------------------------------------
**Description:** Show the Strings view that will display all the printable strings in the program. A combo-box will allow the user to choose whether they want to view strings from the entire binary or from specific segments and sections.    

**Steps:** Windows -> Strings  

**Shortcut:** :kbd:`Shift` + :kbd:`F12`

Show Types
----------------------------------------
**Description:** Show the Types widget in which you can define, load, export and manage data types such as Structures and Enums,  

**Steps:** Windows -> Types  

Add a new instance of the Decompiler Widget
----------------------------------------------
**Description:** Create a new instance of the Decompiler widget in order to view multiple decompiled functions using multiple supported decompilers.

**Steps:** Windows -> Add Decompiler  

Add a new instance of the Disassembly Widget
----------------------------------------------
**Description:** Create a new instance of the Disassembly widget in order to view one or multiple locations at the same time.   

**Steps:** Windows -> Add Disassembly  

Add a new instance of the Graph Widget
----------------------------------------
**Description:** One of the main views of Cutter allows you to navigate inside functions in a graph mode. This view displays the flow of a function where each node on the graph represents a basic block in the function. The edges coming-to and getting-out of the blocks represent the control flow. The menu item will create a new instance of the Graph widget in order to view one or multiple locations at the same time. 

**Steps:** Windows -> Add Graph  

Add a new instance of the Hexdump Widget
-------------------------------------------
**Description:** Create a new instance of the Hexdump widget in order to view one or multiple locations at the same time.   

**Steps:** Windows -> Add Hexdump  

**Shortcut:** :kbd:`Shift` + :kbd:`G`
 

Show Comments
----------------------------------------
**Description:** Show the comments widgets in order to view the automatic and user-defined comments in this session.  

**Steps:** Windows -> Comments  

Show Console
----------------------------------------
**Description:** Open the integrated Rizin console. This will allow you to execute Rizin commands straight from Cutter.   

**Steps:** Windows -> Console  

**Shortcut:** :kbd:`:` or :kbd:`Ctrl` + :kbd:`\``
