View Menu
==============================


Refresh contents
----------------------------------------
**Description:** In some cases, not all the displayed information on Cutter's widgets will be up-to-date, for example - after defining a new function from the integrated radare2 console. By refreshing the contents, Cutter will fetch the most up to date information from the session and will update the different views.

***Note:** In the future, Cutter will be aware to any underlying change and will update everything automatically. This is currently a work-in-progress.*


**Steps:** View -> Refresh Contents  

Reset to default layout
----------------------------------------
**Description:** Reset the current :doc:`layout</user-docs/preferences/layout>` to the default layout provided by Cutter.

**Steps:** View -> Reset to default layout

Reset to default settings
----------------------------------------
**Description:** Reset the current settings to the default settings defined by Cutter.  

**Steps:** View -> Reset Settings  

Lock and Unlock panels
----------------------------------------
**Description:** Allow or disable moving and closing of different widgets. Uncheck this option to prevent accidentally modifying current layout.

**Steps:** View -> Unlock Panels  

Show Tabs at the Top
----------------------------------------
**Description:** Toggle the position of the tab bar.  

**Steps:** View -> Show Tabs at the Top  

Grouped dock dragging
----------------------------------------
**Description:** When enabled, dragging a widget will also drag the widgets which are grouped to it. You can drag a specific widget from a group by dragging from the tab itself and not from the title bar. Disable this option to always drag individual widgets.   

**Steps:** View -> Grouped dock dragging  


Zoom In
----------------------------------------
**Description:** Zoom-In inside different widgets such as Graph, Disassembly and Hexdump.     

**Steps:** View -> Zoom -> Zoom In  

**Shortcut:** :kbd:`Ctrl` + :kbd:`+`  

Zoom Out
----------------------------------------
**Description:** Zoom-Out inside different widgets such as Graph, Disassembly and Hexdump.   

**Steps:** View -> Zoom -> Zoom Out  

**Shortcut:** :kbd:`Ctrl` + :kbd:`-`  

Reset Zoom
----------------------------------------
**Description:** Reset the zoom to its default size.   

**Steps:** View -> Zoom -> Reset  

**Shortcut:** :kbd:`Ctrl` + :kbd:`=`


Manage layouts
----------------------------------------
**Description:**  Rename and delete saved :doc:`layouts</user-docs/preferences/layout>`.

**Steps:** View -> Manage layouts , select layout, choose command

Save layout
----------------------------------------
**Description:** Save the current :doc:`layout</user-docs/preferences/layout>` with a given name. A layout includes the set of currently opened widgets, their position, and some properties.

**Steps:** View -> Save Layout , enter a layout name in the dialog.

Layouts
----------------------------------------
**Description:** Load the settings from the selected :doc:`layout</user-docs/preferences/layout>` into the current layout. Loading a layout will not cause it to automatically be modified. To do that you must use the `Save layout`_ command.

**Steps:** View -> Layouts ->  layout name
