Command line options
====================

Synopsis
--------

**Cutter** [*options*] [<*filename*>]


Options
-------

.. option:: <filename>

   Filename to open. If not specified file selection dialog will be shown.

.. option:: -h, --help

   Displays help on commandline options.

.. option:: --help-all

   Displays help including Qt specific options.

.. option:: -v, --version

   Displays version information.

.. option:: -A, --anal <level>

   When opening a file automatically perform analysis at given level. Requires
   :option:`<filename>` to be specified. Following levels are available:

   **0**
     No analysis.

   **1**
     aaa

   **2**
     aaaa (experimental)

.. option:: -F, --format <name>

   Force using a specific file format (bin plugin)

.. option:: -B, --base <base address>

   Load binary at a specific base address

.. option:: -i <file>

   Run  script file

.. option:: --pythonhome <PYTHONHOME>

   PYTHONHOME to use for embedded python interpreter

.. option:: --no-output-redirect

   Disable output redirection. Some of the output in console widget will not
   be visible. Use this option when debuging a crash or freeze and output
   redirection is causing some messages to be lost.
