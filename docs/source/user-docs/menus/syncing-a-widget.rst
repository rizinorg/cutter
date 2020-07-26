Syncing a Widget
==============================

Sync Widget to an Offset
----------------------------------------
**Description:** By default, widgets like Disassembly, Graph, 
Decompiler and Hexdump are synchronized with each other, pointing to the same address. A change in one widget will affect the others. We consider this feature as a Global-Seek. By un-syncing a widget, the widget will be independent of the global seek and will have its own seek location. Navigating in an unsynced widget will not change the Global-Seek of the rest of the widgets, and vice versa - Changing the Global-Seek will not affect the unsynced widget. Multiple widgets can be unsynced independently.  

**Steps:**  -> Sync/unsync offset