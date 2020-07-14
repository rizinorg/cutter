Development environment setup
=============================

This page contains recommendations and tips on how to better setup different IDEs for Cutter development.

.. toctree::
   :maxdepth: 2

General advice
--------------
Everyone has their own preferences for their favorite IDE or code editor.
There are no strict requirements for using a specific one for Cutter development.

Any IDE with good CMake integration should work well.

For most development builds it is recommended to use CUTTER_USE_BUNDLED_RADARE2=ON Cmake option. It is the easiest way of ensuring that compatible r2 version is used and dealing with multiple r2 versions when working with multiple Cutter branches. On Linux in case of multiple r2 versions without CUTTER_USE_BUNDLED_RADARE2 PKG_CONFIG_PATH environment variable can be used to select desired radare2 installation.

Qt Creator has a builtin visual form editor but not having it in other IDEs is not a major problem. It is also available as standalone tool called Qt Designer and you can configure the file associations so that .ui files are opened using it. Depending on the .ui file and changes you want to make it is sometimes easier to perform them by editing the .ui file as a text file. .ui files are XML files. Most code editors should have some support for XML highlighting and possibly block folding.

Linux
-----
On a rolling-release distro or a somewhat recent version of traditional distro like Ubuntu 18.04 it should be possible to get all the dependencies from official repository.

Windows
-------

Qt Creator
----------
QT Creator is an open source IDE made by same developers as Qt.

Even though Cutter has qmake project cutter.pro it is recommended to use the CMake project in QTCreator.
QTCreator support for CMake is as good as qmake one but not all Cutter project configuration options are available in qmake project and in future Cutter qmake project might be removed.

Pros and Cons
~~~~~~~~~~~~~

- builtin help for Qt API
- builtin .ui file editor (Qt Designer - visual form editor)
- Viewing source files that are not directly part of the project (R2 source code) is somewhat inconvenient.

Project setup
~~~~~~~~~~~~~
Instructions made based on Qt Creator 4.12.4 the steps might slightly differ between the versions.

- Get all the dependencies and download Cutter source code as described in :doc:`build instructions <../building>` .
- `File/Open File or Project..` and select `cutter/src/CMakeList.txt`
- Select kit and press "Configure Project"
- Configuration step might fail due to r2 not being found, that's normal
- Click "Projects" button with wrench icon on the left side of screen
- Click `Add/Boolean` in the "CMake" section
- Enter "CUTTER_USE_BUNDLED_RADARE2" as key name and change the value to ON, in earlier QT Creator versions it is necessary to do this during initial kit selection and configuration step.
- Click "Apply Configuration Changes", configuration should succeed now. In case of errors inspect the output log.

Either in "Projects/Code Style/C++" or "Tools/Options/C++/Code Style" select "Qt [built-in]". It should be selected by default unless you have used QTCreator for other projects. Cutter Coding style is almost identical to Qt one. This will help with using correct indentation type and basic formatting without running code formatter.

To configure AStyle for formatting a file go to "Tools/Options/Beautifier/Artistic Style". If necessary specify the path to astyle executable. "Use file *.astylerc defined in project files" doesn't seem to be working reliably so it is necesarry to use "Use specific config file" option. Cutter astyle configuration is stored in "cutter/src/Cutter.astylerc".

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Click "Projects" button on the left side of screen and then select "Build". All the project CMake options are listed and can be edited there in a graphical editor.

Editing Qt .ui files
~~~~~~~~~~~~~~~~~~~~
Double clicking a .ui file in a file list opens it inside a visual editor. If you want to make changes that are easier to do by editing .ui file as text - right click the file and select "Open With/Plain Text Editor". Switching from visual form editor back to code editor mode will open the .ui file in read only mode with a warning "This file can only be edited in Design mode". To edit use the same steps as described before.

VSCode
-------
VSCode is an open source code editor made by Microsoft.

Pros and Cons
~~~~~~~~~~~~~

- Many plugins to help with different tasks. Can be used for different kind of projects.
- Good fallback mechanism for files that are not directly part of project.

Recommended plugins
~~~~~~~~~~~~~~~~~~~
- `C/C++ <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>`_ official C++ support plugin made by Microsoft
- `CMake Tools <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>`_ provides CMake project integration. Originally developed by vector-of-bool currently maintained by Microsoft.
- `CMake <https://marketplace.visualstudio.com/items?itemName=twxs.cmake>`_ CMake language support when editing CMake files. Does not replace the previous CMake plugin. They provide non-overlapping functionality and should be used together.

Project setup
~~~~~~~~~~~~~
- `File/Open Folder` select the folder in which you cloned Cutter
- If the recommend plugins are already installed in the corner you will see a popup "Would you like to configure project 'cutter'? Source: CMake Tools (Extension)" Click Yes.
- If you initially dismissed the configuration window or didn't have the plugins installed yet - open command pallet using `Ctrl+Shift+P` and select `Cmake: Configure`
- In the kit selection choose "[Unspecified]" unless you have more specific needs.
- If you see error "CMakeList.txt" was not found in the root of folder cutter" chose "Locate" and specify the path to `cutter/src/CMakeLists.txt`
- `Ctrl+Shift+P`/`CMake: Edit CMake Cache`, find the line ``CUTTER_USE_BUNDLED_RADARE2:BOOL=OFF`` and change it to ON.

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
After the first configuration `Ctrl+Shift+P`/`CMake: Edit CMake Cache` opens a text editor with all CMake options. Cutter specific ones mostly start with "CUTTER".

CLion
-----

Visual Studio
-------------