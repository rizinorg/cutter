Common Issues
=============

This page lists common issues encountered by users.

AppImage Crashes
----------------

If the Linux AppImage binary crashes upon startup, make sure your
``LD_LIBRARY_PATH`` environment variable is empty.
For a detailed explanation, see the issue `#579 <https://github.com/radareorg/cutter/issues/579>`__

Keyboard Layout Issue
---------------------

Some people report that they have keyboard issues. Usually it is because
the Xorg layout is wrong. You can check it with: ``setxkbmap -query``
Most of the time using ``setxkbmap us`` solves the issue, but it might
not be enough and require a more advanced Xorg configuration.
