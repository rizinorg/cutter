Building
========

.. note::

 If you just want to use the latest Release version of Cutter, please note
 that we provide pre-compiled binaries for Windows, Linux and macOS on
 our `release page. <https://github.com/radareorg/cutter/releases/latest>`_

Getting the Source
------------------

Make sure you've ``git`` installed in your system (`Installation guide <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_) and do the following:

.. code-block:: sh

   git clone --recurse-submodules https://github.com/radareorg/cutter

This will clone the Cutter source and it's dependencies(radare2, etc.)
under **cutter** and you should see the following dir structure:

.. code-block:: sh

    cutter/-|
            |-docs/     # Cutter Documentation
            |-radare2/  # radare2 submodule
            |-src/      # Cutter Source Code

Following sections assume that **cutter** is your working dir. (if not, do ``cd cutter``)

Building on Linux
-----------------

Requirements
~~~~~~~~~~~~

On Linux, you will need:

* build-essential
* cmake
* meson
* libzip-dev
* libzlib-dev
* qt5
* qt5-svg
* pkgconf
* curl*
* python-setuptools*
* KSyntaxHighlighter**
* graphviz**

 `*` Recommended while building with ``make``/``Cmake``.

 `**` Optional. If present, these add extra features to Cutter. See `CMake Building Options`_.

On Debian-based Linux distributions, all of these essential packages can be installed with this single command:

::

   sudo apt install build-essential cmake meson libzip-dev zlib1g-dev qt5-default libqt5svg5-dev qttools5-dev qttools5-dev-tools

.. note::
 For Ubuntu 18.04 and lower, ``meson`` should be installed with ``pip install --upgrade --user meson``.

On Arch-based Linux distributions:

::

   sudo pacman -Syu base-devel cmake meson qt5-base qt5-svg qt5-tools

Building Steps
~~~~~~~~~~~~~~

The recommended way to build Cutter on Linux is by using CMake. Simply invoke CMake to build Cutter and its dependency radare2.

.. code:: sh

   mkdir build && cd build
   cmake -DCUTTER_USE_BUNDLED_RADARE2=ON ../src
   cmake --build .

If your operating system has a newer version of CMake (> v3.12) you can use this cleaner solution:

.. code:: sh

   cmake -S src -B build -DCUTTER_USE_BUNDLED_RADARE2=ON
   cmake --build build


.. note::

   If you are interested in building Cutter with support for Python plugins,
   Syntax Highlighting, Crash Reporting and more,
   please look at the full list of `CMake Building Options`_.


After the build process is complete, you should have the ``Cutter`` executable in the **build** dir.
You can now execute Cutter like this:

.. code:: sh

   ./build/Cutter


Building on Windows
-------------------

Requirements
~~~~~~~~~~~~

Cutter works on Windows 7 or newer.
To compile Cutter it is necessary to have the following installed:

* A version of Visual Studio (2015, 2017 and 2019 are supported)
* CMake
* Qt

Recommended Way
~~~~~~~~~~~~~~~

To build Cutter on Windows machines using CMake,
you will have to make sure that the executables are available
in your ``%PATH%`` environment variable.

Note that the paths below may vary depending on your version of Qt and Visual Studio.

.. code:: batch

   set CMAKE_PREFIX_PATH=c:\Qt\qt-5.6.2-msvc2013-x86\5.6\msvc2013\lib\cmake
   cd src
   mkdir build
   cd build
   cmake-gui ..

Click ``Configure`` and select your version of Visual Studio from the list,
for example ``Visual Studio 14 2015``.
After the configuration is done, click ``Generate`` and you can open
``Cutter.sln`` to compile the code as usual.


Building with Meson
~~~~~~~~~~~~~~~~~~~

There is another way to compile Cutter on Windows if the one above does
not work or does not suit your needs.

Additional requirements:

-  Ninja build system
-  Meson build system

Download and unpack
`Ninja <https://github.com/ninja-build/ninja/releases>`__ to the Cutter
source root directory (ie. **cutter** - working dir).

Note that in the below steps, the paths may vary depending on your version of Qt and Visual Studio.

Environment settings (example for x64 version):

.. code:: batch

    :: Export MSVC variables
    CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
    :: Add qmake to PATH
    SET "PATH=C:\Qt\5.10.1\msvc2015_64\bin;%PATH%"
    :: Add Python to PATH
    SET "PATH=C:\Program Files\Python36;%PATH%"

Install Meson:

.. code:: batch

   python -m pip install meson

To compile Cutter, run:

.. code:: batch

   CALL prepare_r2.bat
   CALL build.bat


--------------

Building with Qmake
-------------------

Using QtCreator
~~~~~~~~~~~~~~~

One standard way is to simply load the project inside QtCreator.
To do so, open QtCreator and on the welcome screen click on "Open Project",
and finally select the ``cutter/src/Cutter.pro`` file.
QtCreator will then allow you to directly edit the source code and build the project.

.. note::

   On **Windows**, for the ``.pro`` file to be compiled successfully, it is required
   to run ``prepare_r2.bat`` beforehand.

Compiling on Linux / macOS
~~~~~~~~~~~~~~~~~~~~~~~~~~

The easiest way, but not the one we recommend, is to simply run ``./build.sh`` from the root directory,
and let the magic happen. The script will use ``qmake`` to build Cutter.
The ``build.sh`` script is meant to be deprecated and will be deleted in the future.

If you want to manually use qmake, follow these steps:

.. code:: sh

   mkdir build; cd build
   qmake ../src/Cutter.pro
   make
   cd ..

Additional Steps for macOS
~~~~~~~~~~~~~~~~~~~~~~~~~~

On macOS you will also have to copy the launcher bash script:

.. code:: sh

   mv Cutter.app/Contents/MacOS/Cutter Cutter.app/Contents/MacOS/Cutter.bin
   cp ../src/macos/Cutter Cutter.app/Contents/MacOS/Cutter && chmod +x Cutter.app/Contents/MacOS/Cutter


--------------

CMake Building Options
----------------------

Note that there are some major building options available:

* ``CUTTER_USE_BUNDLED_RADARE2`` automatically compile Radare2 from submodule.
* ``CUTTER_ENABLE_PYTHON`` compile with Python support.
* ``CUTTER_ENABLE_PYTHON_BINDINGS`` automatically generate Python Bindings with Shiboken2, required for Python plugins!
* ``CUTTER_ENABLE_KSYNTAXHIGHLIGHTING`` use KSyntaxHighlighting for code highlighting.
* ``CUTTER_ENABLE_GRAPHVIZ`` enable Graphviz for graph layouts.
* ``CUTTER_ENABLE_CRASH_REPORTS`` is used to compile Cutter with crash handling system enabled (Breakpad).

These options can be enabled or disabled from the command line arguments passed to CMake.
For example, to build Cutter with support for Python plugins, you can run this command:

::

   cmake -B build -DCUTTER_ENABLE_PYTHON=ON -DCUTTER_ENABLE_PYTHON_BINDINGS=ON

Or if one wants to explicitly disable an option:

::

   cmake -B build -DCUTTER_ENABLE_PYTHON=OFF


--------------

Compiling Cutter with Breakpad Support
--------------------------------------

If you want to build Cutter with crash handling system, you will want to first prepare Breakpad.
For this, simply run one of the scripts (according to your OS) from root Cutter directory:
    
.. code:: sh

   source scripts/prepare_breakpad_linux.sh # Linux
   source scripts/prepare_breakpad_macos.sh # MacOS
   scripts/prepare_breakpad.bat # Windows
   
Then if you are building on Linux you want to change ``PKG_CONFIG_PATH`` environment variable
so it contains ``$CUSTOM_BREAKPAD_PREFIX/lib/pkgconfig``. For this simply run

.. code:: sh

   export PKG_CONFIG_PATH="$CUSTOM_BREAKPAD_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"


--------------

Troubleshooting
---------------

* **Cmake can't find Qt**

    Cmake: qt development package not found

Depending on how Qt installed (Distribution packages or using the Qt
installer application), CMake may not be able to find it by itself if it
is not in a common place. If that is the case, double-check that the
correct Qt version is installed. Locate its prefix (a directory
containing bin/, lib/, include/, etc.) and specify it to CMake using
``CMAKE_PREFIX_PATH`` in the above process, e.g.:

::

   rm CMakeCache.txt # the cache may be polluted with unwanted libraries found before
   cmake -DCMAKE_PREFIX_PATH=/opt/Qt/5.9.1/gcc_64 ..

* **Radare2's libr_*.so cannot be found when running Cutter**

   ./Cutter: error while loading shared libraries: libr_lang.so: cannot open shared object file: No such file or directory

The exact r2 .so file that cannot be found may vary. On some systems, the linker by default uses RUNPATH instead of RPATH which is incompatible with the way r2 is currently compiled. It results in some of the r2 libraries not being found when running cutter. You can verify if this is the problem by running `ldd ./Cutter`. If all the r2 libraries are missing you have a different problem.
The workaround is to either add the `--disable-new-dtags` linker flag when compiling Cutter or add the r2 installation path to LD_LIBRARY_PATH environment variable.

::

   cmake -DCMAKE_EXE_LINKER_FLAGS="-Wl,--disable-new-dtags"  ..

* **r_*.h: No such file or directory**

    r_util/r_annotated_code.h: No such file or directory

If you face an error where some header file starting with ``r_`` is missing, you should check the **radare2** submodule and
make sure it is in sync with upstream **Cutter** repo. Simply run:

::

   git submodule update --init --recursive

* **r_core development package not found**

If you installed radare2 and still encounter this error, it could be that your
``PATH`` environment variable is set improperly (doesn’t contain
``/usr/local/bin``). You can fix this by adding the radare2 installation dir to
your ``PATH`` variable.

macOS specific solutions:

On macOS, that can also be, for example, due to ``Qt Creator.app``
being copied over to ``/Applications``. To fix this, append
``:/usr/local/bin`` to the ``PATH`` variable within the *Build
Environment* section in Qt Creator. See the screenshot below should you
encounter any problems.

You can also try:

-  ``PKG_CONFIG_PATH=$HOME/bin/prefix/radare2/lib/pkgconfig qmake``
-  ``PKG_CONFIG_PATH=$HOME/cutter/radare2/pkgcfg qmake`` (for a newer
   version and if the radare2 submodule is being built and used)

.. image:: images/cutter_path_settings.png

You can also install radare2 into ``/usr/lib/pkgconfig/`` and then
add a variable ``PKG_CONFIG_PATH`` with the value ``/usr/lib/pkgconfig/``.

* **macOS libjpeg error**

On macOS, Qt5 apps fail to build on QtCreator if you have the ``libjpeg``
installed with brew. Run this command to work around the issue:

::

   sudo mv /usr/local/lib/libjpeg.dylib /usr/local/lib/libjpeg.dylib.not-found