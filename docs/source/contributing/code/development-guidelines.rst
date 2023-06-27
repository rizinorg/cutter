
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

Calling a Rizin Command
~~~~~~~~~~~~~~~~~~~~~~~~~

There are multiple ways to call a Rizin command: 

- ``CutterCore::cmdj(<command>)`` - To be used with json commands like ``cmdj("agj")`` or ``cmdj("aflj")``. 
  This is the command we used to fetch structured data from Rizin.
  
- ``CutterCore::cmdRaw(<command>)`` - Executes a single Rizin command 
  without going through Rizin shell functionality like output redirects, grep, and multiple command parsing.

The command then returns its output. This should be used when a command doesn't have output or the output should be handled as-is. If possible, using the JSON variation with ``cmdj`` is always preferred.
  
- ``CutterCore::cmdRawAt(<command>, <address>)`` - Executes a single Rizin command in a given address and returns the output. This helps avoiding weird strings concatenation like ``cmd("ph " + hash + " @ " + QString::num(address))``.
  
- ``CutterCore::cmd()`` - *(Discouraged)* Only use it when ``cmdj`` or ``cmdRaw`` cannot be used. This is used for complex commands using concatenation of several commands (``px 5; pd 7; afl;``), for grepping (``pd 5~call``). for commands inside commands (``?e `afn.```) and so on.
  This is also used when the output is complex and is not parsed correctly in ``cmdRaw``.
  Make sure to carefully sanitize user-controlled variables that are passed to the command, to avoid unexpected command injections. 

Generally, if one needs to retrieve information from a Rizin command, it
is preferred to use the JSON API.

Example:

.. code:: cpp

   CutterJson array = Core()->cmdj("pdj 1 @ main");

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

In general, we follow a slightly customized version of `the official Qt guidelines <https://wiki.qt.io/Qt_Coding_Style>`__ 
to format the code. Before sending a pull request, you will need to use `clang-format`<https://clang.llvm.org/docs/ClangFormat.html>`__ (version 8 or newer)
to format the code. The command line for formatting the code according
to the style is:

.. code:: bash

   clang-format -style=file -i src/filename.cpp

If your changes were done on many files across the codebase, you can use this oneliner to tun ``clang-format`` on the entire 'src' directory:

.. code:: bash

   find ./src -regex '.*\.\(cpp\|h\)' -exec clang-format -style=file -i {} \;

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
       MyClassThatExists *classInstance;

And then include the class header inside your .cpp so you can use that class.

If you need something in the source file (.cpp) that is not a class member,
then add the include in the source file.

The includes must be ordered from local to global. That is, first include
any local header file (with double quotes like `#include "common/Helpers.h"`.
Then, after an empty newline, include Qt definitions like
`#include <QShortcut>`.
Finally, include the standard C++ headers you need.

Includes must be sorted by alphabetical order.

Docstrings
~~~~~~~~~~

Our API reference is generated using Doxygen, so when it comes to
function documentation, please use the following format:

.. code:: cpp

   /**
    * @brief Add a new param to the accumulator
    */
   virtual void accumulate(RefreshDeferrerParams params) =0;

Loops
~~~~~

We use the C++11 foreach loop style, which means any “foreach” loop should
look like:

.. code:: cpp

   for (QJsonValue value : importsArray) {
       doSomething(value);
   }

nullptr
~~~~~~~

Please do not use ``0`` nor ``Q_NULLPTR``, only use ``nullptr``.

Example:

.. code:: cpp

   QObject *object = nullptr;

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


General Coding Advices
----------------------

Functions Documentation
~~~~~~~~~~~~~~~~~~~~~~~

You can find the class documentation in the API Reference menu item.

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

* `Signals & Slots <https://doc.qt.io/qt-5/signalsandslots.html>`__
* `Model/View Programming <https://doc.qt.io/qt-5/model-view-programming.html>`__ - read this if you are going to work with a list or table-like widgets
* `QAction <https://doc.qt.io/qt-5/qaction.html#details>`__
