Development environment setup
=============================

This page contains recommendations and tips on how to better setup different IDEs for Cutter development.


General advice
--------------
Everyone has their own preferences for their favorite IDE or code editor.
There are no strict requirements for using a specific one for Cutter development.
Any IDE with good CMake integration should work well.

For most development builds, unless you are working on packaging issues, it is recommended to use ``CUTTER_USE_BUNDLED_RIZIN=ON`` Cmake option. It is the easiest way to ensure that a compatible Rizin version is used, and helps you deal with different versions of Rizin when working with multiple Cutter branches. On Linux, in case you have multiple Rizin versions without ``CUTTER_USE_BUNDLED_RIZIN``, the ``PKG_CONFIG_PATH`` environment variable can be used to select the desired Rizin installation.

While `Qt Creator`_ has a builtin visual form and widget editor, not having it in other IDEs is not a major problem. It is also available as a standalone tool called Qt Designer and you can configure the file associations so that ``.ui`` files are opened using it. Depending on the ``.ui`` file and changes you want to make, it is sometimes easier to perform them by editing the ``.ui`` file as a text file. Essentially, ``.ui`` files are XML files. Most code editors should have some support for XML highlighting and possibly block folding.

The following instructions and recommendations assume that you have already download Cutter source and obtained required dependencies as described in :doc:`/building`.

Linux
-----

On a rolling-release distribution or a somewhat recent version of traditional distributions like Ubuntu 18.04, it should be possible to get all the dependencies from the official repository. There might be some problems with PySide2 and Shiboken2 but it can be easily disabled and isn't necessary for most work on Cutter. Don't try to install PySide using pip.

Windows
-------

Assuming you have a sufficiently powerful computer, a nice way of getting and configuring Qt for Cutter is to use `vcpkg <https://github.com/Microsoft/vcpkg>`_.
For a quick test, the exact versions of libraries used by Cutter release packages can be obtained from `cutter-deps <https://github.com/rizinorg/cutter-deps/releases>`_ but they don't contain debug
versions of libraries so they are not suitable for more serious Cutter development on Windows.

Qt Creator
----------
QT Creator is an open-source IDE made by the same developers as Qt.

Pros and Cons
~~~~~~~~~~~~~

- builtin help for Qt API
- builtin .ui file editor (Qt Designer - visual form editor)
- builtin helper for displaying Qt types in the debugger
- Viewing source files that are not directly part of the project (Rizin source code) is somewhat inconvenient.
- The simplest way of installing on non-Linux operating systems require login with Qt account

Project setup
~~~~~~~~~~~~~
The following instructions were made based on version 4.12.4 of Qt Creator. The steps might slightly differ between the versions.

- Go to :menuselection:`File --> Open File or Project..` and select :file:`cutter/CMakeList.txt`
- Select kit and press :guilabel:`Configure Project`
- Configuration step might fail due to Rizin not being found, that's normal
- Click :guilabel:`Projects` button with wrench icon on the left side of the screen
- Click :menuselection:`Add --> Boolean` in the CMake section
- Enter ``CUTTER_USE_BUNDLED_RIZIN`` as a key name and change the value to ON. In earlier Qt Creator versions it is necessary to do this during the initial kit selection and configuration step.
- Click :guilabel:`Apply Configuration Changes`: The configuration should succeed now. In case of errors inspect the output log.


Formatting using clang-format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To configure ``clang-format`` for formatting a file you will need to use the built-in Beautifier plugin. `Follow the instructions <https://doc.qt.io/qtcreator/creator-beautifier.html>`_ on Qt Creator's website to enable the plugin and configure it to run ``clang-format`` when saving a file. In the clang-format options page choose "Use predefined style: File".

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Click on the "Projects" button on the left side of the screen and then select "Build". All the project CMake options are listed and can be edited there in a graphical editor.

Editing Qt .ui files
~~~~~~~~~~~~~~~~~~~~
Double-clicking a ``.ui`` file in a file list opens it inside a visual editor. If you want to make changes that are easier to do by editing ``.ui`` file as text - right-click the file and select :menuselection:`Open With --> Plain Text Editor`. Switching from visual form editor back to code editor mode will open the ``.ui`` file in read-only mode with the following warning "This file can only be edited in Design mode". To edit use the same steps as described before.

VS Code
-------
`VS Code <https://github.com/Microsoft/vscode>`_ is an open-source code editor made by Microsoft.

Pros and Cons
~~~~~~~~~~~~~

- A large number of plugins
- A good fallback mechanism for files that are not directly part of a project.

Recommended plugins
~~~~~~~~~~~~~~~~~~~
- `C/C++ <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>`_ - The official C++ support plugin made by Microsoft
- `CMake Tools <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools>`_ - Provides CMake project integration. Originally developed by vector-of-bool and currently maintained by Microsoft.
- `CMake <https://marketplace.visualstudio.com/items?itemName=twxs.cmake>`_ - CMake language support when editing CMake files. Does not replace the previous CMake plugin. They provide non-overlapping functionality and should be used together.

Project setup
~~~~~~~~~~~~~
- :menuselection:`File --> Open Folder...` and select the folder in which you cloned Cutter
- Install the recommended plugins.
- Once the `CMake Tools` plugin is installed, in the corner you will see a popup asking you "Would you like to configure project 'cutter'? Source: CMake Tools (Extension)". Click Yes.
- In the kit selection popup, choose :guilabel:`[Unspecified]` unless you have more specific needs.
- If you initially dismissed the configuration window or didn't have the plugins installed yet - open command-palette using :kbd:`Ctrl-Shift-P` and select :guilabel:`Cmake: Configure`

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
After the first configuration :kbd:`Ctrl-Shift-P`/:guilabel:`CMake: Edit CMake Cache` opens a text editor with all CMake options. Cutter specific ones mostly start with "CUTTER".

.. note::
    ``CUTTER_USE_BUNDLED_RIZIN`` option is also defined in ``.vscode/settings.json`` file and it will be overridden from there. It is set to ``ON`` by default as it is recommended during development.

.. _vscode-debug-setup:

Formatting using clang-format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The C/C++ extension we recommended earlier supports source code formatting using clang-format which is included with the extension. Use :kbd:`Ctrl-Shift-I` to format the document or :kbd:`Ctrl-K Ctrl-F` to only format the selection. We recommend to configure auto-formatting via the settings. `Follow the instructions <https://code.visualstudio.com/docs/cpp/cpp-ide#_code-formatting>`_ on VS Code's website.

Building, Running, Debugging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Build and running commands are available in the status bar at the bottom and in the Command Palette menu (:kbd:`Ctrl-Shift-P`) named ``CMake: Build F7``, ``CMake: Run Without Debugging Shift+F5``, and ``CMake Debug Ctrl + F5``.
Shortcuts can be viewed in the :kbd:`Ctrl-Shift-P` menu. They don't match default VS Code ones since those depend on :file:`tasks.json``.

Running and debugging launches the executable without any arguments. Command-line arguments can be passed to the debug
executable by creating a ``.vscode/launch.json`` configuration. Read the `documentation <https://code.visualstudio.com/docs/cpp/launch-json-reference>`_  for more information. Instead of creating :file:`launch.json` manually it can be created from template: :kbd:`Ctrl-Shift-P`/:menuselection:`Debug: Select and Start Debugging --> Add configuration.. --> C,C++: (gdb) Launch`.

To setup gdb pretty printers for Qt types on Linux, download the scripts from `Kdevelop <https://github.com/KDE/kdevelop/tree/master/plugins/gdb/printers>`_. In the :file:`~/.gdbinit` file add the following code:


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
- Go to :menuselection:`File --> Open` and select the folder in which you cloned Cutter
- Go to :menuselection:`File --> Settings --> Build, Execution, Deployment --> CMake`. In the :guilabel:`CMake Options` field enter ``-DCUTTER_USE_BUNDLED_RIZIN=ON``
- Open :file:`cutter/CMakeLists.txt` using the project file list on the left side of the screen
- A yellow bar with a message :guilabel:`CMake project is not loaded` should appear, click :guilabel:`Load CMake project`

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Go to :menuselection:`File --> Settings --> Build,Execution,Deployment --> CMake`. CMake options are specified the same way as on command-line ``-DOPTION_NAME=VALUE``.

Formatting using clang-format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Clion provides you with an easy way to format files with ``clang-format``. Follow the `documentation <https://www.jetbrains.com/help/clion/clangformat-as-alternative-formatter.html>`_ on their website to learn how to enable formatting with ``clang-format``.

Building, Running, Debugging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Follow the `Clion documentation <https://www.jetbrains.com/help/clion/qt-tutorial.html#debug-renderers>`_ for how to configure Qt type debugger renderers. If you are using the MSVC toolchain
it can use :file:`qt5.natvis`. In rest of the cases you can use ``.gdbinit`` or ``..ldbinit`` based approach similar to one described for :ref:`VSCode setup<vscode-debug-setup>`

Editing Qt .ui files
~~~~~~~~~~~~~~~~~~~~
Default CLion behavior for opening .ui files is `somewhat buggy <https://youtrack.jetbrains.com/issue/CPP-17197>`_. Double-clicking the file does nothing, but it can be opened by dragging it to the text editor side.
This can be somewhat improved by changing `file association <https://www.jetbrains.com/help/clion/creating-and-registering-file-types.html>`_. Open :menuselection:`File --> Settings --> Editor --> File Types` and change type association of \*.ui files from :guilabel:`Qt UI Designer Form` to either "XML" or :guilabel:`Files Opened in Associated Applications`.
The first one will open it within CLion as an XML file and the second will use the operating system configuration.

Visual Studio
-------------
Visual Studio Community edition is available for free and can be used for contributing to open source projects.

It is recommended to use the latest Visual Studio version 2019 because it has the best CMake integration.
Older VS versions can be used but CMake integration isn't as good. With those, it might be better to generate Visual Studio
project from CMake project using the command-line or :command:`cmake-gui` and opening the generated Visual Studio project instead of opening the
CMake project directly.

Visual Studio supports many different languages and use-cases. Full installation takes a lot of space. To keep the size minimal during installation
select only component called "Desktop development with C++". Don't worry too much about missing something.
Additional components can be later added or removed through the VS installer which also serves as an updater and package manager for Visual Studio components.

Pros and Cons
~~~~~~~~~~~~~
- good debugger
- medium amount of plugins
- completely closed source
- Windows only

Project setup
~~~~~~~~~~~~~
- Open folder in which you cloned Cutter source using Visual Studio
- Open CMake settings configurator using either :menuselection:`Project --> CMake Settings` or by clicking :guilabel:`Open the CMake Settings Editor` in the overview page.
- Check ``CUTTER_USE_BUNDLED_RIZIN`` option
- If you are using vcpkg, Visual Studio should detect it automatically. The list of CMake options in the configurator should have some referring to vcpkg. If they are not there, specify the path to vcpkg toolchain file in the :guilabel:`CMake toolchain file` field.
- If you are not using vcpkg, configure the path to Qt as mentioned in :ref:`windows CMake instructions<building:Building on Windows>`. You can specify the CMake flag in :guilabel:`CMake command arguments:` field.
- To Ensure that VS debugger can display Qt types in a readable way, it is recommended to install `Qt Visual Studio Tools <https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2019>`_ plugin. It will create a :file:`Documents/Visual Studio 2019/Visualizers/qt5.natvis` file. Once :file:`qt5.natvis` has been created you can uninstall the plugin.

Changing CMake configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Open :menuselection:`Project --> CMake Settings`. CMake options can be modified either in graphical table editor, as a command-line flag or by switching to the JSON view.

Formatting using clang-format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Visual Studio supports ``clang-format`` by default so you should not do anything special. It will simple use the existing ``_clang-format`` file from Cutter's root directory. If you wish to configure how and when Visual Studio will use ``clang-format``, you can do this from :menuselection:`Tools --> Options --> Text Editor --> C/C++ --> Formatting`.

Editing Qt .ui files and Qt integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
By default Visual Studio will open ``.ui`` files as XML text documents. You can configure to open it using Qt Designer by right-clicking and selecting :guilabel:`Open With...`.

There is a  Qt plugin for Visual Studio from Qt. It isn't very useful for Cutter development since it is aimed more at helping with Qt integration into Visual Studio projects.
It doesn't do much for CMake based projects. The biggest benefit is that it automatically installs :file:`qt5.natvis` file for more readable displaying of Qt types in the debugger.
