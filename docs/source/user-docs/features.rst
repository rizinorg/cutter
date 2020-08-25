Features
=============
You can read about the exciting features in Cutter here.

Decompiler
-------------
A decompiler is a sophisticated transformation engine that will analyze binaries and attempt to create a high-level representation of the machine code in the executable binary file. In other words, it tries to reconstruct the source code from which the binary was compiled in the first place.

Here's an image that compares one of the decompiler Cutter supports with the Cutter's disassembler.

.. image:: ../images/decompiler_vs_disassembly.png


Cutter provides an interface that supports plugins of multiple decompilers including Ghidra, R2Dec, and RetDec. The interface receives data from the decompiler in the form annotations and using them Cutter presents a context-sensitive decompiler widget. `Click here <https://github.com/radareorg/cutter-plugins#decompilers>`_ to know more about the decompilers we support.

.. _here: https://github.com/radareorg/cutter-plugins#decompilers

Of the three decompiler plugins that we support, the one that's officially maintained by the developers of Cutter and Radare2 is R2Ghidra. :doc:`Click here <menus/decompiler-context-menu>` to know more about all the functionalities we provide in the decompiler.