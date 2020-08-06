Analysis Options
================

Cutter will use the underlying radare2 analysis options to analyze a binary. These options are usually 
configured when the binary is first loaded. However, they can be later changed using the Analysis 
dialog, and a new analysis that takes these options into account can then be performed.

Analysis Dialog
---------------

.. image:: ../../images/analysis_dialog.png
    :alt: Analysis dialog

**Description:** The Analysis dialog allows setting some radare2's analysis options. The supported options are described
below.

Clicking on the "Analyze Program" button will trigger an analysis of the current binary with radare2's ``aaa``
command, taking into account the configured values of the analysis options.

**Steps to open:** ``Edit -> Preferences -> Analysis``

Speculatively set a name for the functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Try to name functions without symbols by using artifacts in the functions such as API calls and strings.

**Configuration variable:** ``anal.autoname``


Search for new functions following already defined functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Cutter will check if there is a candidate for a new function following an already defined one, as the compiler usually
state them together. This option is taking the advantages of both Recursive Analysis and Linear Sweep into some kind of a hybrid mode. For each discovered function, the analysis will try to check for a function-prologue, usually following a gap of null bytes or ``cc`` bytes. This will help with discovering functions which are not referenced in the program. As such, it will make the analysis slower and prone to false-positives.

**Configuration variable:** ``anal.hasnext``


Create references for unconditional jumps
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When encountering unconditional jumps during the analysis, Cutter will add a code-reference even if it is not guaranteed
that the jump will take place.

**Configuration variable:** ``anal.jmp.ref``


Analyze jump tables in switch statements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When encountering switch statements during analysis, continue and analyze the target blocks of the jump tables.

**Configuration variable:** ``anal.jmp.tbl``


Analyze ``push`` + ``ret`` as ``jmp``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When performing analysis of a function, treat the sequence of ``push`` followed by ``ret`` instruction as a ``jmp``.
This can help Cutter to continue the analysis to target of the ``jmp``.

**Configuration variable:** ``anal.pushret``


Show verbose information when performing analysis
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When enabled, Cutter will print warnings it encountered while preforming analysis. These warnings can help the user
understand if anything went wrong in the analysis. This function is not only helpful when trying to perform a full
analysis of the program, but also when trying to analyze and define new functions.

**Configuration variable:** ``anal.verbose``


Verbose output from type analysis
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Print warnings encountered while preforming type analysis. These warnings can help the user understand if anything went
wrong in the analysis.

**Configuration variable:** ``anal.types.verbose``
