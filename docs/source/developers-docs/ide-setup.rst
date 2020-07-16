Development environment setup
=============================

This page contains recommendations and tips on how to better setup different IDEs for Cutter development.

.. contents::

General advice
--------------
Everyone has their own preferences for their favorite IDE or code editor.
There are no strict requirements for using a specific one for Cutter development.
Any IDE with good CMake integration should work well.

For most development builds unless you are working on packaging issues it is recommended to use ``CUTTER_USE_BUNDLED_RADARE2=ON`` Cmake option. It is the easiest way of ensuring that compatible r2 version is used and dealing with multiple r2 versions when working with multiple Cutter branches. On Linux in case of multiple r2 versions without ``CUTTER_USE_BUNDLED_RADARE2`` ``PKG_CONFIG_PATH`` environment variable can be used to select desired radare2 installation.

Qt Creator has a builtin visual form editor but not having it in other IDEs is not a major problem. It is also available as standalone tool called Qt Designer and you can configure the file associations so that .ui files are opened using it. Depending on the .ui file and changes you want to make it is sometimes easier to perform them by editing the .ui file as a text file. .ui files are XML files. Most code editors should have some support for XML highlighting and possibly block folding.

All instructions assume that you have already download cutter source and obtained required dependencies as described in :doc:`../building`

Linux
-----

On a rolling-release distribution or a somewhat recent version of traditional distribution like Ubuntu 18.04 it should be possible to get all the dependencies from official repository. There might
be some problems with PySide2 and Shiboken2 but it can be easily disabled and isnt' necessary for most work on Cutter. Don't try to install PySide using pip.

Windows
-------

Assuming you have a sufficiently powerful computer a nice way of getting and configuring Qt for Cutter is to use `vcpkg <https://github.com/Microsoft/vcpkg>`_.
For a quick test the exact versions of libraries used by Cutter release packages can be obtained from `cutter-deps <https://github.com/radareorg/cutter-deps/releases>`_ but they don't contain debug
versions of libraries so they are not suitable for more serious Cutter development on Windows.

Qt Creator
----------
QT Creator is an open source IDE made by same developers as Qt.

Even though Cutter has qmake project cutter.pro it is recommended to use the CMake project in QTCreator.
QTCreator support for CMake is as good as qmake one but not all Cutter project configuration options are available in qmake project and in future Cutter qmake project might be removed.

Pros and Cons
~~~~~~~~~~~~~

- builtin help for Qt API
- builtin .ui file editor (Qt Designer - visual form editor)
- builtin helper for displaying Qt types in the debugger
- Viewing source files that are not directly part of the project (R2 source code) is somewhat inconvenient.
- The simplest way of installing on non-Linux operating systems require login with Qt account

Project setup
~~~~~~~~~~~~~
Instructions made based on Qt Creator 4.12.4 the steps might slightly differ between the versions.

- ``File/Open File or Project..`` and select ``cutter/src/CMakeList.txt``
- Select kit and press ``Configure Project``
- Configuration step might fail due to r2 not being found, that's normal
- Click ``Projects`` button with wrench icon on the left side of screen
- Click ``Add/Boolean`` in the CMake section
- Enter ``CUTTER_USE_BUNDLED_RADARE2`` as key name and change the value to ON, in earlier QT Creator versions it is necessary to do this during initial kit selection and configuration step.
- Click ``Apply Configuration Changes``, configuration should succeed now. In case of errors inspect the output log.

Either in ``Projects/Code Style/C++`` or ``Tools/Options/C++/Code Style`` select ``Qt [built-in]``. It should be selected by default unless you have used QTCreator for other projects. Cutter Coding style is almost identical to Qt one. This will help with using correct indentation type and basic formatting without running code formatter.

To configure AStyle for formatting a file go to ``Tools/Options/Beautifier/Artistic Style``. If necessary specify the path to astyle executable. ``Use file \*.astylerc defined in project files`` doesn't seem to be working reliably so it is necessary to use ``Use specific config file`` option. Cutter astyle configuration is stored in ``cutter/src/Cutter.astylerc``.

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Click "Projects" button on the left side of screen and then select "Build". All the project CMake options are listed and can be edited there in a graphical editor.

Editing Qt .ui files
~~~~~~~~~~~~~~~~~~~~
Double clicking a .ui file in a file list opens it inside a visual editor. If you want to make changes that are easier to do by editing .ui file as text - right click the file and select "Open With/Plain Text Editor". Switching from visual form editor back to code editor mode will open the .ui file in read only mode with a warning "This file can only be edited in Design mode". To edit use the same steps as described before.

VSCode
-------
`VSCode <https://github.com/Microsoft/vscode>`_ is an open source code editor made by Microsoft.

Pros and Cons
~~~~~~~~~~~~~

- Large amount of plugins
- Good fallback mechanism for files that are not directly part of project.

Recommended plugins
~~~~~~~~~~~~~~~~~~~
- `C/C++ <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>`_ official C++ support plugin made by Microsoft
- `CMake Tools <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools>`_ provides CMake project integration. Originally developed by vector-of-bool currently maintained by Microsoft.
- `CMake <https://marketplace.visualstudio.com/items?itemName=twxs.cmake>`_ CMake language support when editing CMake files. Does not replace the previous CMake plugin. They provide non-overlapping functionality and should be used together.

Project setup
~~~~~~~~~~~~~
- ``File/Open Folder`` select the folder in which you cloned Cutter
- If the recommend plugins are already installed in the corner you will see a popup "Would you like to configure project 'cutter'? Source: CMake Tools (Extension)" Click Yes.
- If you initially dismissed the configuration window or didn't have the plugins installed yet - open command pallet using :kbd:`Ctrl-Shift-P` and select ``Cmake: Configure``
- In the kit selection choose ``[Unspecified]`` unless you have more specific needs.
- If you see error "CMakeList.txt was not found in the root of folder cutter" chose ``Locate`` and specify the path to ``cutter/src/CMakeLists.txt``
- :kbd:`Ctrl-Shift-P`/``CMake: Edit CMake Cache``, find the line ``CUTTER_USE_BUNDLED_RADARE2:BOOL=OFF`` and change it to ON.
- Download Qt type visualizer for VS debugger from


Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
After the first configuration :kbd:`Ctrl-Shift-P`/``CMake: Edit CMake Cache`` opens a text editor with all CMake options. Cutter specific ones mostly start with "CUTTER".

Building, Running, Debugging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Build and running commands are available in the bar at bottom left and in :kbd:`Ctrl-Shift-P` menu named ``CMake: Build F7``, ``CMake: Run Without Debugging Shift+F5``, and ``CMake: Debug Ctrl+F5``.
Shortcuts can be viewed in the :kbd:`Ctrl-Shift-P` menu. They don't match default VSCode ones since the depend on ``tasks.json``.

Running and debugging launches the executable without any arguments. Command line arguments can be passed to the debug
executable by creating a ``.vscode/launch.json`` configuration. Read `documentation <https://code.visualstudio.com/docs/cpp/launch-json-reference>`_  for more information. Instead of creating ``launch.json`` it can be created from template: :kbd:`Ctrl-Shift-P`/``Debug: Select and Start Debugging/Add configuration../C,C++: (gdb) Launch``.

To setup gdb pretty printers for Qt types on Linux download scripts from `Kdevelop <https://github.com/KDE/kdevelop/tree/master/plugins/gdb/printers>`_. In the ``~/.gdbinit`` file add following code


.. code-block:: python

    python
    import sys

    sys.path.insert(0, '/path/to/folder/with/pretty_printer_scripts')
    from qt import register_qt_printers
    register_qt_printers (None)

    end
    set print pretty 1


CLion
-----
`CLion <https://www.jetbrains.com/clion/>`_ is a C and C++ IDE from the popular software development tool maker - JetBrains.


Pros and Cons
~~~~~~~~~~~~~

- Medium amount of plugins, many first-party plugins made by JetBrains for their IntelliJ based IDE family
- There is no free version
- Takes some time to analyze the files after opening a project. Switching between .cpp and corresponding .h file may for the first time may take a few seconds.

Project setup
~~~~~~~~~~~~~
- ``File/Open`` select the folder in which you cloned Cutter
- ``File/Settings/Build,Execution,Deployment/CMake`` in the ``CMake Options`` field enter ``-DCUTTER_USE_BUNDLED_RADARE2=ON``
- Open ``cutter/src/CMakeLists.txt`` using project file list on the left side of screen
- A yellow bar with message "CMake project is not loaded" should appear, click "Load CMake project"

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
``File/Settings/Build,Execution,Deployment/CMake`` CMake options are specified the same way as on command-line ``-DOPTION_NAME=VALUE``.

Editing Qt .ui files
~~~~~~~~~~~~~~~~~~~~
Default CLion behavior for opening .ui files is `somewhat buggy <https://youtrack.jetbrains.com/issue/CPP-17197>`_. Double clicking the file does nothing, but it can be opened by dragging it to text editor side.
This can be somewhat improved by changing `file association <https://www.jetbrains.com/help/clion/creating-and-registering-file-types.html>`_. Open ``File/Settings/Editor/File Types`` and change to change type association of \*.ui files from "Qt UI Designer Form" to either "XML" or "Files Opened in Associated Applications".
First one will open it within CLion as XML file and the second will use the operating system configuration.

Visual Studio
-------------
Visual Studio Community edition is available for free and can be used for contributing to open source projects.

It is recommended to use the latest Visual Studio version 2019 because it has best CMake integration.
Older VS versions can be used but CMake integration isn't as good. With those it might be better to generate Visual Studio
project from CMake project using command-line or cmake-gui and opening the generated Visual Studio project instead of opening
CMake project directly.

Visual Studio supports many different languages and use-cases. Full installation takes a lot of space. To keep the size minimal during installation
select only component called "Desktop development with C++". Don't worry too much about missing something.
Additional components can be later added or removed through the VS installer which also serves as an updater and package manager for Visual Studio components.

Pros and Cons
~~~~~~~~~~~~~
- good debugger
- medium amount of plugins
- completely closed source

Project setup
~~~~~~~~~~~~~
- Open folder in which you cloned Cutter source using Visual Studio
- Open CMake settings configurator using either ``Project/CMake Settings`` or by clicking ``Open the CMake Settings Editor`` in overview page.
- Check `CUTTER_USE_BUNDLED_RADARE2` options
- If you are using vcpkg Visual Studio should detect it automatically. List of CMake options in the configurator should have some referring to VCPKG. If they are not there specify the path to vcpkg toolchain file in the "CMake toolchain file" field.
- If you are not using VCPKG configure path to Qt as mentioned in :ref:`windows CMake instructions<building:Building on Windows>`. You can specify the CMake flag in "CMake command arguments:" field.
- To Ensure that VS debugger can display Qt types in a readable way it is recommended to install `Qt Visual Studio Tools <https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2019>`_ plugin. It will create a ``Documents/Visual Studio 2019/Visualizers/qt5.natvis`` file. Once ``qt5.natvis`` has been created you can uninstall the plugin.

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Open ``Project/CMake Settings``. CMake options can be modified either in graphical table editor, as a command-line flag or by switching to JSON view.

Editing Qt .ui files and Qt integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
By default Visual Studio will open .ui files as XML text document. You can configure to open it using QT Designer by right clicking and selecting ``Open With...``.

There is a  Qt plugin for Visual Studio from Qt. It isn't very useful for Cutter development since it is aimed more at helping with Qt integration into Visual Studio projects.
It doesn't do much for CMake based projects. The biggest benefit is that it automatically installs ``qt5.natvis`` file for more readable displaying of Qt types in debugger.