File Menu
==============================

New Instance of Cutter
----------------------------------------
**Description:** Open a new instance of Cutter to start a new session. This option will open a new window of Cutter without exiting the current session.  

**Steps:** File -> New Window  

**Shortcut:** :kbd:`Ctrl` + :kbd:`N`  

Map a new file
----------------------------------------
**Description:** Cutter allows you to map the contents of other files into the same I/O space used to contain the loaded binary. The new contents can be placed at random or specific offsets.
Specifically, Cutter is able to open files and map portions of them at random or specific places in memory. It is the perfect basic tooling to reproduce an environment like a core file, a debug session, or a framework by also loading and mapping all the libraries and files the binary depends on.  

**Steps:** File -> Map File  

**Shortcut:** :kbd:`Ctrl` + :kbd:`M`  

Import a PDB file
----------------------------------------
**Description:** Cutter allows you to load additional debugging information by loading external PDB files. Unlike other platforms, Cutter does not rely on Windows API to parse a PDB files, thus they can be loaded on any other supported platform like Linux or macOS.    

**Steps:** File -> Import PDB  

Enable Write mode
-----------------------
**Description:** This option will enable Write mode and allow you to patch and edit the binary on disk. **Please note** that when Write mode is enabled, each change you make to the binary in Cutter will be applied to the original file on disk. In order not to take unnecessary risks, consider using Cache mode instead.

**Steps:** File -> Set mode -> Write mode


Enable Cache mode
-----------------------
**Description:** This option will enable Cache mode and allow you to patch and edit the binary **without** applying the changes to the file on disk. Unlike in Write mode, in Cache mode, the changes you make to the binary in Cutter will not be applied to the original file on disk unless you specifically committing them using the "Commit changes" menu item. This is safer than using Write mode because there is no risk to lose important data.

**Steps:** File -> Set mode -> Cache mode


Enable Read-Only mode
------------------------
**Description:** This option is available when files are opened in Write or Cache modes. When Read-Only mode is enabled, no patches and editions to the file are allowed. This is the default mode for files in Cutter, unless specified otherwise by the user, by either enabling Write or Cache mode.

**Steps:** File -> Set mode -> Read-Only mode

Commit changes from cache
----------------------------
**Description:** Apply the changes performed in Cache mode to the file on disk. Cache mode will not apply the changes and patches made unless the user clicks "Commit changes". To automatically apply every change to the file on disk, use the less-safer Write mode.

**Steps:** File -> Commit changes


Save project
----------------------------------------
**Description:** Save your session to a project. If no project file assigned to your session, the "Save as..." dialog will open.  

**Steps:** File -> Save  

**Shortcut:** :kbd:`Ctrl` + :kbd:`S`  

Save project as...
----------------------------------------
**Description:** Save the current state of your session, including function names, comments, and more.
On the saving dialog, choose the project type that fits your needs.  
***Note:** This feature is currently unstable and under constructions.*


**Steps:** File -> Save As...  

Export to code
----------------------------------------
**Description:** Export the entire binary in different formats that later can be used in your favorite programming language. The feature supports many formats such as Python arrays, java, several C array formats, javascript, and more.   
***Note:** This isn't a decompilation feature.*


**Steps:** File -> Export as code  

Run a radare2 script
----------------------------------------
**Description:** Cutter allows you to execute radare2 scripts to automate task or transfer information.   

**Steps:** File -> Run Script  

Quit Cutter
----------------------------------------
**Description:** Quit and exit your current session of Cutter. On exit, you'll be asked whether you want to save your session in order to avoid losing data.   

**Steps:** File -> Quit  

**Shortcut:** :kbd:`Ctrl` + :kbd:`Q`