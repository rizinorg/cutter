Crash Handling System
=====================

Cutter uses `Breakpad <https://github.com/google/breakpad>`__ as a backend
for crash handling.

Crash Handling System is disabled by default to not interfere with developers while debugging.
To enable this system, set the ``CUTTER_ENABLE_CRASH_REPORTS`` build option.

Solution Description
--------------------

There are only 2 source files:

* ``CrashHandler.h``
* ``CrashHandler.cpp``

And the API is very simple: One function, ``initCrashHandler()``, enables the Crash Handling System if
``CUTTER_ENABLE_CRASH_REPORTS`` is true, otherwise it does nothing.

As soon as a signal is raised, ``crashHandler(int signum)`` is called with the signal's code as an argument.
This function first writes a crash dump to the operating system's temporary directory to catch core and
memory state as it was at the moment of the crash.

Then the crash dialog is shown:

.. image :: /images/crash-dialog.png

If the user chooses to create a crash dump, the prepared dump is moved to the directory specified by the user.
And then the success dialog is shown:

.. image :: /images/success-dump-dialog.png
