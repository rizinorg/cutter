
Cutter Development Guidelines
===============================

.. note::
   New to Cutter development? Check out our :doc:`tutorial for new developers <getting-started>`.


Common Usage
--------------

CutterCore Class
~~~~~~~~~~~~~~~~

This is the main class where every link with r2 is made. It is *unique*
across the whole process. To access it, simply call ``Core()``.

Example:

.. code:: cpp

   Core()->getOffset();

Calling a radare2 Command
~~~~~~~~~~~~~~~~~~~~~~~~~

There are multiple ways to call a radare2 command: 

- ``CutterCore::cmdj(<command>)`` - To be used with json commands like ``cmdj("agj")`` or ``cmdj("aflj")``. 
  This is the command we used to fetch structured data from radare2.
  
- ``CutterCore::cmdRaw(<command>)`` - Executes a single radare2 command 
  without going through radare2 shell functionality like output redirects, grep, and multiple command parsing.

The command then returns its output. This should be used when a command doesn't have output or the output should be handled as-is. If possible, using the JSON variation with ``cmdj`` is always preferred.
  
- ``CutterCore::cmdRawAt(<command>, <address>)`` - Executes a single radare2 command in a given address and returns the output. This helps avoiding weird strings concatenation like ``cmd("ph " + hash + " @ " + QString::num(address))``.
  
- ``CutterCore::cmd()`` - *(Discouraged)* Only use it when ``cmdj`` or ``cmdRaw`` cannot be used. This is used for complex commands using concatenation of several commands (``px 5; pd 7; afl;``), for grepping (``pd 5~call``). for commands inside commands (``?e `afn.```) and so on.
  This is also used when the output is complex and is not parsed correctly in ``cmdRaw``.
  Make sure to carefully sanitize user-controlled variables that are passed to the command, to avoid unexpected command injections. 

Generally, if one needs to retrieve information from a radare2 command, it
is preferred to use the JSON API.

Example:

.. code:: cpp

   QJsonArray array = Core()->cmdj("pdj 1 @ main").array();

Seek the Current File
~~~~~~~~~~~~~~~~~~~~~

To modify radare2 seek use ``CutterCore::seek(const RVA offset)``. This
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
so your widget refreshes its output when radare2 seek is modified
(switching to another function, etc.).

Coding Style
------------

In general, we follow `the official Qt guidelines <https://wiki.qt.io/Qt_Coding_Style>`__ to
format the code. If in doubt, you can use `AStyle
2.06 <https://sourceforge.net/projects/astyle/files/astyle/astyle%202.06/>`__
to format the code. The command line for formatting the code according
to the style is:

.. code:: bash

   astyle --project=src/Cutter.astylerc src/filename.cpp

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

Connecting Signals
~~~~~~~~~~~~~~~~~~

To connect a signal to a slot, this is the preferred syntax:

.. code:: cpp

   connect(sender, &QObject::destroyed, this, &MyObject::objectDestroyed);

This syntax performs compile-time type checks and allows the use of lambda
functions. Other approaches for connecting signals silently break at runtime.

General Coding Advices
----------------------

Functions Documentation
~~~~~~~~~~~~~~~~~~~~~~~

You can find the class documentation in the API Reference menu item.

Updating the Git Submodules
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Git submodules play a major part in Cutter. This, because Cutter is powered
by radare2, its parent project, and it tries to stay up-to-date with its
recent version, which allows us to implement new features, and enjoy bug
fixes and performance improvements on radare2. Often, we need to update
the radare2 submodule or the others, to push their most recent
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

More likely, you'll only need to update the radare2 submodule.
In order to update one submodule individually, use the following code:

.. code:: sh

   cd radare2
   git checkout master && git pull
   cd ..
   git add radare2
   git commit -m "Update radare2 submodule"


Useful Resources (Qt Development)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* `Signals & Slots <https://doc.qt.io/qt-5/signalsandslots.html>`__
* `Model/View Programming <https://doc.qt.io/qt-5/model-view-programming.html>`__ - read this if you are going to work with a list or table-like widgets
* `QAction <https://doc.qt.io/qt-5/qaction.html#details>`__
