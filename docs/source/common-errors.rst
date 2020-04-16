Common errors
=============

This page lists common issues encountered by users.

AppImage crashes
----------------

If the linux AppImage binary crashes upon startup, make sure your
``LD_LIBRARY_PATH`` environment variable is empty.
`#579 <https://github.com/radareorg/cutter/issues/579>`__

--------------

Compilation error
-----------------

r_core development package not found
~~~~~~~~~~~~~~~~~~

If you installed radare2 and still encounter this error, it could be that your
``PATH`` environment variable is set improperly (doesn’t contain
``/usr/local/bin``). That can be, for example, due to ``Qt Creator.app``
being copied over to ``/Applications``. To fix this, append
``:/usr/local/bin`` to the ``PATH`` variable within the *Build
Environment* section in Qt Creator. See the screenshot below should you
encounter any problems.

On macOS, you can also try:

-  ``PKG_CONFIG_PATH=$HOME/bin/prefix/radare2/lib/pkgconfig qmake``
-  ``PKG_CONFIG_PATH=$HOME/cutter/radare2/pkgcfg qmake`` (for modern
   version and if radare2 was installed like the submodule)

.. image:: images/cutter_path_settings.png

Or radare2 libs could be installed to ``/usr/lib/pkgconfig/``, so you
can add variable ``PKG_CONFIG_PATH`` with value ``/usr/lib/pkgconfig/``

--------------

OS X libjpeg error
~~~~~~~~~~~~~~~~~~

On macOS, QT5 apps fail to build on QtCreator if you have the libjpeg lib
installed with brew. Run this command to work around the issue:

::

   sudo mv /usr/local/lib/libjpeg.dylib /usr/local/lib/libjpeg.dylib.not-found

Keyboard layout issue
---------------------

Some people report that they have keyboard issues. Usually it is because
the Xorg layout is wrong. You can check it with: ``setxkbmap -query``
Most of the time using ``setxkbmap us`` solves the issue, but it might
not be enough and require a more advanced Xorg configuration.
