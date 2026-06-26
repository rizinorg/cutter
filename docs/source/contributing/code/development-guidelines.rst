
Cutter Development Guidelines
===============================

.. note::
   New to Cutter development? Check out our :doc:`tutorial for new developers <getting-started>`.


Common Usage
--------------

CutterCore Class
~~~~~~~~~~~~~~~~

This is the main class where every link with Rizin is made. It is *unique*
across the whole process. To access it, simply call ``Core()``.

Example:

.. code:: cpp

   Core()->getOffset();


Seek the Current File
~~~~~~~~~~~~~~~~~~~~~

To modify Rizin seek use ``CutterCore::seek(const RVA offset)``. This
is important because it will emit a
``CutterCore::seekChanged(RVA offset)`` signal. Never ever call
``cmd("s offset")``;

Example:

.. code:: cpp

   Core()->seek(0x00C0FFEE);

.. note::

 Cutter also supports a silent seek which doesn't trigger the ``seekChanged`` event and doesn't add new entries to the seek history.


Creating a Widget
~~~~~~~~~~~~~~~~~

Make sure to connect the ``CutterCore::seekChanged(RVA offset)`` signal
so your widget refreshes its output when Rizin seek is modified
(switching to another function, etc.).

Coding Style
------------

clang-format
~~~~~~~~~~~~

In general, we follow a slightly customized version of `the official Qt guidelines <https://wiki.qt.io/Qt_Coding_Style>`__ 
to format the code. Before sending a pull request, you will need to use `clang-format <https://clang.llvm.org/docs/ClangFormat.html>`__ (version 8 or newer)
to format the code. The command line for formatting the code according
to the style is:

.. code:: bash

   clang-format -style=file -i src/filename.cpp

If your changes were done on many files across the codebase, you can use this oneliner to run ``clang-format`` on the entire ``src`` directory:

.. code:: bash

   find ./src -regex '.*\.\(cpp\|h\)' -exec clang-format -style=file -i {} \;

clang-tidy
~~~~~~~~~~

Beyond formatting, we use `clang-tidy <https://clang.llvm.org/extra/clang-tidy/>`__ (version 13 or newer) to catch potential style violations.

To run ``clang-tidy``, first configure your build to generate ``compile_commands.json`` and run the autogen tools:

.. code:: bash

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUTTER_QT=6 -DCUTTER_USE_BUNDLED_RIZIN=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
     
    cmake --build build --target Cutter_autogen

Run ``clang-tidy`` on modified files relative to the latest commit

.. code:: bash

    git diff -U0 --no-color HEAD~1 | clang-tidy-diff.py -p1 -path build/

Similar to ``clang-format``, If your changes were done on many files across the codebase, you can use this oneliner to run ``clang-tidy`` on the entire 'src' directory. The following command excludes third-party and auto generated files:

.. code:: bash

    run-clang-tidy -p build ".*src/(?!(themes|bindings|fonts|img|translations|Cutter_autogen)).*\.(cpp|h)$"

``clang-tidy`` can also attempt to fix style violations using the ``-fix`` flag. However these may not always be perfect. Make sure to verify the fixes before opening a pull request:

.. code:: bash

    git diff -U0 --no-color HEAD~1 | clang-tidy-diff.py -fix -p1 -path build/

Python scripts
~~~~~~~~~~~~~~

If you don't want to run manual commands, cutter also provides scripts for running ``clang-format`` and ``clang-tidy`` in the ``scripts`` directory

.. code:: bash

    python scripts/clang-format.py -h
    usage: clang-format.py [-h] [-C CLANG_FORMAT] [-c] [-v] [-f FILE] [-d DIFF]

    Clang format the cutter project

    options:
      -h, --help            show this help message and exit
      -C, --clang-format CLANG_FORMAT
                            path of clang-format
      -c, --check           enable the check mode
      -v, --verbose         use verbose output
      -f, --file FILE       formats (or checks) only the given file
      -d, --diff DIFF       format all modified file related to branch

.. code:: bash
    
    usage: clang-tidy.py [-h] [-T RUN_CLANG_TIDY] [-p BUILD_PATH] [-j JOBS] [-r REGEX] [-i] [-q]

    clang-tidy regex wrapper

    options:
      -h, --help            show this help message and exit
      -T, --run-clang-tidy RUN_CLANG_TIDY
                            Path of run-clang-tidy binary
      -p, --build-path BUILD_PATH
                            Path to the build directory
      -j, --jobs JOBS       Number of parallel execution jobs
      -r, --regex REGEX     Regex pattern for filtering files
      -i, --fix             Apply fixes automatically
      -q, --quiet           Suppress configuration logs

Below are some of the low level coding conventions that we follow

Variables
~~~~~~~~~

Variable names start with a lowercase letter with each consecutive word starting with an uppercase letter. (**camelBack** case)

.. code:: cpp

   int Height             // Wrong
   string name_of_widget  // Wrong
       
   int height;            // Correct
   string nameOfWidget    // Correct

Avoid meaningless variable names

.. code:: cpp

   int a, b;              // Wrong
   string c;              // Wrong

   int height, width;     // Correct
   string nameOfThis;     // Correct
   Object obj;            // Also Correct

Only first letter of an acronym is uppercase 

.. code:: cpp

   CutterJsonWigdet cutterJSONWidget;    // Wrong

   CutterJsonWigdet cutterJsonWigdet;    // Correct

Variable names don't use leading/trailing underscores, including prefixes like m\_

.. code:: cpp

   class Example {
     private:
       int index_;     // Wrong
       int m_index;    // Wrong

       int index;      // Correct
   };

Global variables follow the same naming conventions

Anonymous Namespace
~~~~~~~~~~~~~~~~~~~

Global entities (variables, functions, structs...) private to a source file (internal linkage) are defined in an anonymous namespace

.. code:: cpp

   namespace {
     int globalCounter = 0;
       
       bool doSomething() {
           ...
           return true;
       }
   };

Functions
~~~~~~~~~

Function names follow the same naming convention as `variables`_. (**camelBack** casing)

.. code:: cpp

   void do_something() {}     // Wrong

   void DoSomething() {}      // Wrong

   void doSomething() {}      // Correct

Unused parameters inside a function are omitted in the definition either by commenting out the parameter name or not writing the name at all.

Avoid using ``Q_UNUSED``

.. code:: cpp
   
    // Bad
    void doSomething(int one, int two) {
        Q_UNUSED(one)
        ...
    }

    // Good
    void doSomething(int /*one*/, int two) {
        ...    
    }

    // Good
    void doSomething(int, int two) {
        ...    
    }

Avoid the use of automatic name based connections. See `Connecting Qt Signals`_.

Classes
~~~~~~~~

First letter of each word is uppercased. (**CamelCase**)

.. code:: cpp

   class memory_dock_widget;   // Wrong
   class memoryDockWidget;     // Wrong

   class MemoryDockWidget;     // Correct

Only first letter of an acronym is uppercase 

.. code:: cpp

   class CutterJSONWidget    // Wrong

   class CutterJsonWidget    // Correct


Member variables follow the same naming conventions defined in `variables`_ that means no leading/trailing underscores or m\_ prefixes

Casting
~~~~~~~

Avoid the use of C style casts, prefer C++ casts (``static_cast``, ``const_cast`` ...)

Use ``qobject_cast`` for ``QObjects``

Smart Pointers
~~~~~~~~~~~~~~

Prefer the use of C++ smart pointers (``unique_ptr``, ``shared_ptr``...)

.. code:: cpp

    RegisterProfileDialog *ui;                        // Bad

    std::unique_ptr<Ui::RegisterProfileDialog> ui;    // Good
    
Braces
~~~~~~

In contrast to the official guidelines of Qt, in Cutter we always use curly braces in conditional statements, even if the body of a conditional statement contains only one line.

.. code:: cpp

   // Wrong
   if (address.isEmpty())
      return false;
   
   // Correct
   if (address.isEmpty()) {
      return false;
   }
   
   // Wrong
   for (int i = 0; i < 10; ++i)
      qDebug("%i", i);
   
   // Correct
   for (int i = 0; i < 10; ++i) {
      qDebug("%i", i);
   }

Loops
~~~~~

We use the C++11 foreach loop style, which means any “foreach” loop should
look like:

.. code:: cpp
   
   // Good - If a copy of each element is required
   for (auto import : importsArray) {
       doSomething(import);
   }
   
   // Good - If copy is not required
   for (auto &import : importsArray) {
       doSomething(import);
   }

   // Good - If no modification is required
   for (const auto &import : importsArray) {
       doSomething(import);
   }

auto
~~~~

Prefer the use of ``auto`` keyword when working with the following:

.. code:: cpp

   // Iterators
   auto it = myMap.find(key);
   for (const auto &import : importsArray) { ... }

   // Lambdas
   auto multiply = [](int a, int b) -> int { return a * b; };

   // Casting
   auto myFloat = static_cast<float>(myInt);
    
   // Initializing using new
   auto *item = new QListWidgetItem(text);

nullptr
~~~~~~~

Please do not use ``0`` nor ``Q_NULLPTR``, only use ``nullptr``.

Example:

.. code:: cpp

    std::unique_ptr<QObject> obj = nullptr;
     
    // Note that this is just an example, unique_ptr constructor initializes the internal pointer to nullptr by default


Connecting Qt Signals
~~~~~~~~~~~~~~~~~~~~~

Use one of the following methods for connecting signals to slots:

.. code:: cpp

   // typically you will make connection in the constructor to a member of current class
   connect(this->ui->button1, &QPushButton::clicked,
           this, &MyObject::buttonClicked); // Good

   // you can also connect directly other object slots
   connect(checkbox, &QCheckBox::toggled, widget, &QWidget::setEnabled); // Good

   // use lambda for passing extra arguments
   connect(button1, &QPushButton::clicked, this, [this](){ foo(getBar()); }); // Good

This syntax performs compile-time type checks and allows the use of lambda
functions. Other approaches for connecting signals can silently break at runtime.

Don't use the older macro based syntax or automatic name based connections.

.. code:: cpp

   // SIGNAL and SLOT macros
   connect(sender, SIGNAL(clicked), this, SLOT(buttonClicked)); // BAD

   // automatic name based connection
   slot:
      void on_actionNew_triggered(); // BAD

   // 3 argument connect without receiver object
   connect(sender, &SomeObject::signal, [this](){ this->foo(getBar()); }); // BAD


Includes
~~~~~~~~

Strive to include only **required** definitions inside header files.
This will avoid triggering additional unnecessary compilations.

If you only need to know that a class exists but don't need the prototype,
you can declare the class like this:

.. code:: cpp

   class MyClassThatExists;

   /** ... **/

   private:
     std::unqiue_ptr<MyClassThatExists> classInstance;

And then include the class header inside your .cpp so you can use that class.

If you need something in the source file (.cpp) that is not a class member,
then add the include in the source file.

The includes must be ordered from local to global. That is, first include
any local header file (with double quotes like `#include "common/Helpers.h"`.
Then, after an empty newline, include Qt definitions like
`#include <QShortcut>`.
Finally, include the standard C++ headers you need.

Includes must be sorted by alphabetical order.

This is automatically handled by running ``clang-format``

Docstrings
~~~~~~~~~~

Our API reference is generated using Doxygen, so when it comes to
function documentation, please use the following format:

.. code:: cpp

   /**
    * @brief Add new parameters to the accumulator
    * @param params The parameters to add
    * @return True if the parameters were added, false otherwise
    */
   bool accumulate(RefreshDeferrerParams params) { ... };

Documenting every function is generally not required. Only document those functions whose purpose is not immediately understandable just by looking at the function name.

.. code:: cpp

    // No need for docs
    int getCount() const;

    // Should provide docs
    bool syncRemote();

It is preferred for classes to have documentation just above the definition, explaining their purpose.

.. code:: cpp

    /**
     * @brief A short description about SomeDialog
     */
    class SomeDialog {
        ...
    };

General Coding Advices
----------------------

Functions Documentation
~~~~~~~~~~~~~~~~~~~~~~~

You can find the class documentation in the `API Reference <https://cutter.re/docs/api.html>`__ menu item.

Updating the Git Submodules
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Git submodules play a major part in Cutter. This, because Cutter is powered
by Rizin, its parent project, and it tries to stay up-to-date with its
recent version, which allows us to implement new features, and enjoy bug
fixes and performance improvements on Rizin. Often, we need to update
the Rizin submodule or the others, to push their most recent
version to Cutter.

You can view the list of all the submodules from the cutter root folder with:

.. code:: sh

   git config --file .gitmodules --get-regexp path | awk '{ print $2 }'

To update all the submodules at once, run these commands from the
cutter root folder:

.. code:: sh

   git submodule foreach git pull origin master
   git add submodule_name_1 submodule_name_2
   git commit -m "Update submodules"

More likely, you'll only need to update the *rizin* submodule.
In order to update one submodule individually, use the following code:

.. code:: sh

   cd rizin
   git checkout dev && git pull
   cd ..
   git add rizin
   git commit -m "Update rizin submodule"


Useful Resources (Qt Development)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* `Signals & Slots <https://doc.qt.io/qt-6/signalsandslots.html>`__
* `Model/View Programming <https://doc.qt.io/qt-6/model-view-programming.html>`__ - read this if you are going to work with a list or table-like widgets
* `QAction <https://doc.qt.io/qt-6/qaction.html#details>`__
