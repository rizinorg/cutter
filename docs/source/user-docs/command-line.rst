Command-line Options
====================

Synopsis
--------

**Cutter** [*options*] [<*filename*> | --project <*project*>]


Options
-------

.. option:: <filename>

   Filename to open. If not specified file selection dialog will be shown.

.. option:: -h, --help

   Displays help on command-line options.

.. option:: --help-all

   Displays help including Qt specific options.

.. option:: -v, --version

   Displays version information.

.. option:: -A, --analysis <level>

   When opening a file automatically perform analysis at a given level. Requires
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

   Run script file

.. option:: -p, --project <file>
  
   Load project file 

.. option:: -w, --writemode

   Open a file in write mode, instead of the default read-only mode.
   When used together with -A/--analysis <level>, it will open a file directly
   in write mode without any further dialog or confirmation.

.. option:: --pythonhome <PYTHONHOME>

   PYTHONHOME to use for the embedded python interpreter

.. option:: --no-output-redirect

   Disable output redirection. Some of the output in the console widget will not
   be visible. Use this option when debugging a crash or freeze and output
   redirection is causing some messages to be lost.

.. option:: --no-plugins

   Start cutter with all plugins disabled. Implies :option:`--no-cutter-plugins` and :option:`--no-rizin-plugins`.

.. option:: --no-cutter-plugins

   Start cutter with cutter plugins disabled.

.. option:: --no-rizin-plugins

   Start cutter with rizin plugins disabled.
