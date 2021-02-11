Building
========

.. note::

 If you just want to use the latest Release version of Cutter, please note
 that we provide pre-compiled binaries for Windows, Linux, and macOS on
 our `release page. <https://github.com/rizinorg/cutter/releases/latest>`_

Getting the Source
------------------

Make sure you've ``git`` installed in your system (`Installation guide <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_) and do the following:

.. code-block:: sh

   git clone --recurse-submodules https://github.com/rizinorg/cutter


This will clone the Cutter source and its dependencies(rizin, etc.)
under **cutter** and you should see the following dir structure:

.. code-block:: sh

    cutter/-|
            |-docs/     # Cutter Documentation
            |-rizin/    # Rizin submodule
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

   sudo pacman -Syu --needed base-devel cmake meson qt5-base qt5-svg qt5-tools

Building Steps
~~~~~~~~~~~~~~

The recommended way to build Cutter on Linux is by using CMake. Simply invoke CMake to build Cutter and its dependency Rizin.

.. code:: sh

   mkdir build && cd build
   cmake -DCUTTER_USE_BUNDLED_RIZIN=ON ..
   cmake --build .

If your operating system has a newer version of CMake (> v3.12) you can use this cleaner solution:

.. code:: sh

   cmake -B build -DCUTTER_USE_BUNDLED_RIZIN=ON
   cmake --build build

If you want to use Cutter with another version of Rizin you can omit ``-DCUTTER_USE_BUNDLED_RIZIN=ON``. Note that using a version of Rizin which isn't the version Cutter is using can cause issues and the compilation might fail.

.. note::

   If you are interested in building Cutter with support for Python plugins,
   Syntax Highlighting, Crash Reporting and more,
   please look at the full list of `CMake Building Options`_.


After the build process is complete, you should have the ``Cutter`` executable in the **build** dir.
You can now execute Cutter like this:

.. code:: sh

   ./build/cutter


Building on Windows
-------------------

Requirements
~~~~~~~~~~~~

Cutter works on Windows 7 or newer.
To compile Cutter it is necessary to have the following installed:

* A version of `Visual Studio <https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16>`_ (2015, 2017 and 2019 are supported)
* `CMake <https://cmake.org/download/>`_
* `Qt 5 <https://www.qt.io/download-qt-installer>`_
* `Meson <https://mesonbuild.com/Getting-meson.html#installing-meson-with-pip>`_
* `Ninja <https://github.com/ninja-build/ninja/releases/latest>`_

Building Steps
~~~~~~~~~~~~~~~

To build Cutter on Windows machines using CMake,
you will have to make sure that the executables are available
in your ``%PATH%`` environment variable.

You can check if the binaries are available by opening PowerShell and 
executing the following commands.

.. code:: powershell

   ninja --version
   meson --version
   cmake --version

If they are not available, you can use PowerShell to add them to your PATH one by one:

.. code:: powershell

   $Env:Path += ";C:\enter\path\here"


Note that the paths below may vary depending on your version of Qt and Visual Studio.

.. code:: powershell
   
   # First, set CMAKE_PREFIX_PATH to Qt5 intallation prefix
   $Env:CMAKE_PREFIX_PATH = "C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5"

   # Then, add the following directory to your PATH
   $Env:Path += ";C:\Qt\5.15.2\msvc2019_64\bin"

   # Build Cutter
   cmake -B build -DCUTTER_USE_BUNDLED_RIZIN=ON
   cmake --build build


After the compilation completes, the ``cutter.exe`` binary will be available in ``.\build\Debug\cutter.exe``.



Building on macOS
-------------------

Requirements
~~~~~~~~~~~~

* XCode
* CMake
* Qt
* meson
* ninja


For basic build all dependencies except XCode can be installed using homebrew:

::

   brew install cmake qt5 meson ninja


Recommended Way for dev builds
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: batch

   mkdir build
   cd build
   cmake .. -DCUTTER_USE_BUNDLED_RIZIN=ON -DCMAKE_PREFIX_PATH=/local/opt/qt5
   make

--------------

CMake Building Options
----------------------

Note that there are some major building options available:

* ``CUTTER_USE_BUNDLED_RIZIN`` automatically compile Rizin from submodule.
* ``CUTTER_ENABLE_PYTHON`` compile with Python support.
* ``CUTTER_ENABLE_PYTHON_BINDINGS`` automatically generate Python Bindings with Shiboken2, required for Python plugins!
* ``CUTTER_ENABLE_KSYNTAXHIGHLIGHTING`` use KSyntaxHighlighting for code highlighting.
* ``CUTTER_ENABLE_GRAPHVIZ`` enable Graphviz for graph layouts.
* ``CUTTER_EXTRA_PLUGIN_DIRS`` List of addition plugin locations. Useful when preparing package for Linux distros that have strict package layout rules.

Cutter binary release options, not needed for most users and might not work easily outside CI environment: 

* ``CUTTER_ENABLE_CRASH_REPORTS`` is used to compile Cutter with crash handling system enabled (Breakpad).
* ``CUTTER_ENABLE_DEPENDENCY_DOWNLOADS`` Enable downloading of dependencies. Setting to OFF doesn't affect any downloads done by Rizin build. This option is used for preparing Cutter binary release packges. Turned off by default.
* ``CUTTER_PACKAGE_DEPENDENCIES`` During install step include the third party dependencies. This option is used for preparing Cutter binary release packges. 


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

* **Rizin's librz_*.so cannot be found when running Cutter**

   ./cutter: error while loading shared libraries: librz_lang.so: cannot open shared object file: No such file or directory

The exact Rizin .so file that cannot be found may vary. On some systems, the linker by default uses RUNPATH instead of RPATH which is incompatible with the way Rizin is currently compiled. It results in some of the Rizin libraries not being found when running cutter. You can verify if this is the problem by running `ldd ./cutter`. If all the Rizin libraries are missing you have a different problem.
The workaround is to either add the `--disable-new-dtags` linker flag when compiling Cutter or add the Rizin installation path to LD_LIBRARY_PATH environment variable.

::

   cmake -DCMAKE_EXE_LINKER_FLAGS="-Wl,--disable-new-dtags"  ..

* **rz_*.h: No such file or directory**

    Eg: rz_util/rz_annotated_code.h: No such file or directory

If you face an error where some header file starting with ``rz_`` is missing, you should check the **rizin** submodule and
make sure it is in sync with upstream **Cutter** repo. Simply run:

::

   git submodule update --init --recursive

* **rz_core development package not found**

If you installed Rizin and still encounter this error, it could be that your
``PATH`` environment variable is set improperly (doesn’t contain
``/usr/local/bin``). You can fix this by adding the Rizin installation dir to
your ``PATH`` variable.

macOS specific solutions:

On macOS, that can also be, for example, due to ``Qt Creator.app``
being copied over to ``/Applications``. To fix this, append
``:/usr/local/bin`` to the ``PATH`` variable within the *Build
Environment* section in Qt Creator. See the screenshot below should you
encounter any problems.

You can also try:

-  ``PKG_CONFIG_PATH=$HOME/bin/prefix/rizin/lib/pkgconfig cmake ...``

.. image:: images/cutter_path_settings.png

You can also install Rizin into ``/usr/lib/pkgconfig/`` and then
add a variable ``PKG_CONFIG_PATH`` with the value ``/usr/lib/pkgconfig/``.

* **macOS libjpeg error**

On macOS, Qt5 apps fail to build on QtCreator if you have the ``libjpeg``
installed with brew. Run this command to work around the issue:

::

   sudo mv /usr/local/lib/libjpeg.dylib /usr/local/lib/libjpeg.dylib.not-found
