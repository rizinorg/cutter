Contributing
============

This page shows some hints about the coding conventions.

*Disclaimer: It is a work in progress and we will provide soon a fully
documented API.*

Coding advices
--------------

CutterCore class
~~~~~~~~~~~~~~~~

This is the main class where every link with r2 is made. It is *unique*
accross the whole process. To access it, simply call ``Core()``.

Example:

.. code:: cpp

   Core()->getOffset();

Calling a radare2 command
~~~~~~~~~~~~~~~~~~~~~~~~~

There are two ways to do it: \* ``CutterCore::cmd()`` *(Discouraged)*
Only use it for commands which yells no output \* ``CutterCore::cmdj()``
To be used with json commands like ``cmdj("agj")`` or ``cmdj("aflj")``.
It is way easier to parse a json output.

Example:

.. code:: cpp

   QJsonArray array = Core()->cmdj("pdj 1 @ main").array();

Seek the current file
~~~~~~~~~~~~~~~~~~~~~

To modify radare2 seek use ``CutterCore::seek(const RVA offset)``. This
is important because it will emit a
``CutterCore::seekChanged(RVA offset)`` signal. Never ever call
``cmd("s offset")``;

Example:

.. code:: cpp

   Core()->seek(0x00C0FFEE);

Creating a widget
~~~~~~~~~~~~~~~~~

Make sure to connect the ``CutterCore::seekChanged(RVA offset)`` signal
so your widget refreshes its output when radare2 seek is modified
(switching to another function, etc.).

General coding guidelines
-------------------------

Coding style
~~~~~~~~~~~~

We follow `these guidelines <https://wiki.qt.io/Qt_Coding_Style>`__ to
format the code. If in doubt, you can use `AStyle
2.06 <https://sourceforge.net/projects/astyle/files/astyle/astyle%202.06/>`__
to format the code. The command line for formatting the code according
to the style is:

.. code:: bash

   astyle --project=src/Cutter.astylerc src/filename.cpp


Docstrings
^^^^^^^^^^

Our API reference is generated using Doxygen, so when it comes to
function documentation, please use the following format:

.. code:: cpp

   /**
    * @brief Add a new param to the accumulator
    */
   virtual void accumulate(RefreshDeferrerParams params) =0;

Loops
^^^^^

We use C++11 foreach loop style which means any “foreach” loop should
look like:

.. code:: cpp

   for (QJsonValue value : importsArray) {
       doSomething(value);
   }

Nullptr
^^^^^^^

Please do not use ``0`` nor ``Q_NULLPTR``, only use ``nullptr``.

Example:

.. code:: cpp

   QObject *object = nullptr;

Connecting signals
^^^^^^^^^^^^^^^^^^

To connect a signal to a slot, this is the preferred way to do it:

.. code:: cpp

   connect(sender, &QObject::destroyed, this, &MyObject::objectDestroyed);

The main reason is that this syntax allows the use of lambda functions.

Functions documentation
~~~~~~~~~~~~~~~~~~~~~~~

You can find the classes documentation in the API Reference menu item.
