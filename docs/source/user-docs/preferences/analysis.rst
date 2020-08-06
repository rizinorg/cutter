Analysis Options
================

Cutter will use the underlying radare2 analysis options to analyze a binary. These options are usually 
configured when the binary is first loaded. However, they can be later changed using the Analysis 
dialog, and a new analysis that takes these options into account can then be performed.

Analysis Dialog
---------------

.. image:: ../../images/analysis_dialog.png
    :alt: Analysis dialog


**Description:** The Analysis dialog allows setting some radare2's analysis options. The supported options are the following: 
- `anal.autoname`
- `anal.hasnext`
- `anal.jmp.ref`
- `anal.jmp.tbl`
- `anal.pushret`
- `anal.types.verbose`
- `anal.verbose`

Clicking on the "Analyze Program" button will trigger an analysis of the current binary with radare2's ``aaa`` command, taking into account the configured values of the analysis options above.

**Steps to open:** ``Edit -> Preferences -> Analysis``
