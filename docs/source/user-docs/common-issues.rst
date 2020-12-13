Common Issues
=============

This page lists common issues encountered by users.

AppImage Crashes
----------------

If the Linux AppImage binary crashes upon startup, make sure your
``LD_LIBRARY_PATH`` environment variable is empty.
For a detailed explanation, see the issue `#579 <https://github.com/rizinorg/cutter/issues/579>`__

Keyboard Layout Issue
---------------------

Some people report that they have keyboard issues. Usually it is because
the Xorg layout is wrong. You can check it with: ``setxkbmap -query``
Most of the time using ``setxkbmap us`` solves the issue, but it might
not be enough and require a more advanced Xorg configuration.

Initial Analysis takes a long time or Cutter UI freezes
-------------------------------------------------------

Cutter and Rizin currently don't work very well with large and very large binaries.
The exact limits depend on the content of the binary, but roughly a few MB can be considered large
and may take a few minutes to analyze. More than 100MB is very large,
analysis with default settings will likely take a very long time and it might occasionally
freeze the UI during usage.

If the analysis takes longer than 5-15 minutes it is recommended to retry it with different
analysis options. In the "Load Options" dialog, move the analysis slider to the right in order to reach
the "Advanced Analysis" view. This view will help you learn more about the options that can
be used to more selectively analyze only the relevant parts of code.
