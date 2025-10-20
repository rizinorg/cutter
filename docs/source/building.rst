Building
========

.. note::

 If you just want to use the latest Release version of Cutter, please note
 that we provide pre-compiled binaries for Windows, Linux, and macOS on
 our `release page <https://github.com/rizinorg/cutter/releases/latest>`_ and
 `CI page <https://nightly.link/rizinorg/cutter/workflows/ccpp/dev>`_ for latest development builds.

This page describes how to do a basic build from the command line. If you are planning to modify Cutter it is recommended to also read our :doc:`development environment setup</contributing/code/ide-setup>`.

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
* qt6
* qt6-svg
* pkgconf
* curl*
* python-setuptools*
* KSyntaxHighlighter**
* graphviz**
* shiboken6**
* pyside6**

 `*` Recommended while building with ``make``/``Cmake``.

 `**` Optional. If present, these add extra features to Cutter. See `CMake Building Options`_.

On Debian-based Linux distributions, all of these essential packages can be installed with this single command:

::

   sudo apt install build-essential cmake meson pkgconf libzip-dev zlib1g-dev qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libqt6svg6-dev libqt6core5compat6-dev libqt6svgwidgets6 qt6-l10n-tools

Depending on your configuration you'll might also need the following:

::

  # When building with CUTTER_ENABLE_GRAPHVIZ (Default)
  sudo apt install libgraphviz-dev

.. note::
 For Ubuntu 20.04 and lower (or in any case you get an error ``Meson version is x but project requires >=y``), ``meson`` should be installed with ``pip install --upgrade --user meson``.

On Arch-based Linux distributions:

::

   sudo pacman -Syu --needed base-devel cmake meson qt6-base qt6-svg qt6-tools

   # When building with CUTTER_ENABLE_KSYNTAXHIGHLIGHTING (Default)
   sudo pacman -Syu --needed syntax-highlighting
   # When building with CUTTER_ENABLE_GRAPHVIZ (Default)
   sudo pacman -Syu --needed graphviz
   # When building with CUTTER_ENABLE_PYTHON and CUTTER_ENABLE_PYTHON_BINDINGS
   sudo pacman -Syu --needed pyside6 shiboken6

On dnf/yum based distributions:

::

   sudo dnf install -y gcc gcc-c++ make cmake meson qt6-qtbase-devel qt6-qtsvg-devel qt6-qttools-devel qt6-qt5compat-devel
   # Optional packages
   sudo dnf install -y graphviz-devel kf6-syntax-highlighting-devel python3-devel shiboken6 python3-pyside6-devel clang


On older Linux systems not supported by QT6 you can use Qt 5.15. Use of Qt5 on operating systems other than Linux is untested.

Building Steps
~~~~~~~~~~~~~~

The recommended way to build Cutter on Linux is by using CMake. Simply invoke CMake to build Cutter and its dependency Rizin.

.. code:: sh

   mkdir build && cd build
   cmake ..
   cmake --build .

If your operating system has a newer version of CMake (> v3.12) you can use this cleaner solution:

.. code:: sh

   cmake -B build
   cmake --build build

If you want to use Cutter with another version of Rizin you can set ``-DCUTTER_USE_BUNDLED_RIZIN=OFF``. Note that using a version of Rizin which isn't the version Cutter is using can cause issues and the compilation might fail.

.. note::

   If you are interested in building Cutter with support for Python plugins,
   Syntax Highlighting and more, please look at the full list of `CMake Building Options`_.


After the build process is complete, you should have the ``Cutter`` executable in the **build** dir.
You can now execute Cutter like this:

.. code:: sh

   ./build/cutter


Making Linux distribution specific packages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When making a distribution specific package, the default install target should give you a good starting point.
It uses CMake built-in functionality and `GNUInstallDirs <https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html?highlight=gnu%20directories>`_ for
installing the executable, desktop file, headers and other files required for plugin compilation. See CMake documentation for adjusting installed file locations and properties.
It shouldn't be necessary to manually copy files from plain build.

It is recommended to build and package rizin as a separate package so that it can be used with or without Cutter. Doing that will also give more control over the way rizin dependencies are handled. We are trying to maintain
compatibility with latest rizin release at the time of Cutter release and making a new Cutter release when new rizin version is released.

If you are packaging Cutter, users will appreciate it if you also package `rz-ghidra <https://github.com/rizinorg/rz-ghidra>`_ and `jsdec <https://github.com/rizinorg/jsdec>`_ decompilers as optional packages.
It should be possible to compile Cutter plugins against proper Cutter installation without having direct access to Cutter source code.

If the names "Cutter" or "cutter" conflict with other packages or their content, "rz-cutter" can be used.

:Configuration for packaging:

* ``-DCMAKE_BUILD_TYPE=Release`` turn on release optimizations, unless your distro has more specific guidelines for common compiler options.
* ``CUTTER_USE_BUNDLED_RIZIN=OFF`` turn off use of rizin from submodule to use previously packaged rizin. Note that keeping it on doesn't install rizin in a way suitable for linux packaging without doing additional manual steps making packaging process more complex. Bundled rizin will also likely conflict with standalone rizin package.
* Correct install prefix. By default CMake will install to /usr/local suitable for user builds. Change it according to your distro packaging guidelines.
* ``CUTTER_ENABLE_PYTHON`` and  ``CUTTER_ENABLE_PYTHON_BINDINGS`` it is recommended to turn on for complete user experience. May require manual path specification on distros with multiple python versions.
* ``CUTTER_ENABLE_GRAPHVIZ`` and ``CUTTER_ENABLE_KSYNTAXHIGHLIGHTING`` optional but nice to have since they are available on most distros.
* ``CUTTER_EXTRA_PLUGIN_DIRS`` use it to specify additional plugin search locations if distro packaging guidelines require you placing them in locations Cutter doesn't use by default.
* ``CUTTER_VERSION_SUFFIX`` can be used to differentiate multiple builds based on same upstream Cutter version, distro specific package build number and similar.
* ``CUTTER_INCLUDE_GIT_HASH=OFF`` By default cutter includes git commit hash in the full version string. If you are building from source tarrball or other source which isn't original git repository you might want to disable this.

Building on Windows
-------------------

Requirements
~~~~~~~~~~~~

Cutter works on Windows 10 or newer.
To compile Cutter it is necessary to have the following installed:

* A version of `Visual Studio <https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16>`_ (2019 or newer)
* `CMake <https://cmake.org/download/>`_
* `Qt 6 <https://www.qt.io/download-qt-installer>`_  See :doc:`development environment setup</contributing/code/ide-setup>` for alternative ways of obtaining Qt that don't require Qt account.
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

   # Add the following directory to your PATH
   $Env:Path += ";C:\Qt\6.7.2\msvc2019_64\bin"

   # Build Cutter
   cmake -B build
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

   brew install cmake qt6 meson ninja


Recommended Way for dev builds
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: batch

   mkdir build
   cd build
   cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt6
   make

--------------

CMake Building Options
----------------------

Note that there are some major building options available:

* ``CUTTER_USE_BUNDLED_RIZIN`` automatically compile Rizin from submodule (Enabled by default).
* ``CUTTER_ENABLE_PYTHON`` compile with Python support, required for Python plugins.
* ``CUTTER_ENABLE_PYTHON_BINDINGS`` automatically generate Python Bindings with Shiboken, required for Python plugins!
* ``CUTTER_ENABLE_KSYNTAXHIGHLIGHTING`` use KSyntaxHighlighting for code highlighting.
* ``CUTTER_ENABLE_GRAPHVIZ`` enable Graphviz for graph layouts.
* ``CUTTER_EXTRA_PLUGIN_DIRS`` List of addition plugin locations. Useful when preparing package for Linux distros that have strict package layout rules.
* ``CUTTER_QT`` Qt major version to use. Defaults to 6. Allowed values: 5, 6. 

Cutter binary release options, not needed for most users and might not work easily outside CI environment: 

* ``CUTTER_ENABLE_DEPENDENCY_DOWNLOADS`` Enable downloading of dependencies. Setting to OFF doesn't affect any downloads done by Rizin build. This option is used for preparing Cutter binary release packges. Turned off by default.
* ``CUTTER_PACKAGE_DEPENDENCIES`` During install step include the third party dependencies. This option is used for preparing Cutter binary release packages. 

For full list of Cutter specific build options and their description see CMakeCache.txt after configuring the project or use a graphical CMake configurator if your IDE provides one.

These options can be enabled or disabled from the command line arguments passed to CMake.
For example, to build Cutter with support for Python plugins, you can run this command:

::

   cmake -B build -DCUTTER_ENABLE_PYTHON=ON -DCUTTER_ENABLE_PYTHON_BINDINGS=ON

Or if one wants to explicitly disable an option:

::

   cmake -B build -DCUTTER_ENABLE_PYTHON=OFF




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

