Crash Handling System
=====================

Cutter uses `Breakpad <https://github.com/google/breakpad>`__ as backend
for crash handling.

Crash Handling System is disabled by default to do not interfere developers from debugging.
To enable this system there is building option ``CUTTER_ENABLE_CRASH_REPORTS``.

Solution description
--------------------

There are only 2 source files:

* ``CrashHandler.h``
* ``CrashHandler.cpp``

And API is very simple: only one function - ``initCrashHandler()`` that enables Crash Handling System if
``CUTTER_ENABLE_CRASH_REPORTS`` is true, otherwise does nothing.

As soon as signal is raised ``crashHandler(int signum)`` is called with signal's code as argument.
This function first of all writes crash dump to OS's Temp directory to catch core and memory state 
as it was at the crash moment.

Then crash dialog is shown:

.. image :: images/crash-dialog.png

If user chose to create crash dump, prepared dump is moved to directory specified by user.
And then success dialog is shown:

.. image :: images/success-dump-dialog.png
