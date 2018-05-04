# Contributing to Cutter

There are many ways you can contribute to cutter.
The easy one is to open issues with bugs you found on the application.
The second one is to fix issues found in the GitHub issues tracker.

## Opening an issue

Make a clear description of the bug/feature, use screenshots, send binaries, etc.

## Contributing to the code

Note that cutter is still under development and many parts of the code are to be improved.

### CutterCore class

This is the main class where every link with r2 is made. It is *unique* accross the whole process. To access it, simply call `Core()`.

Example:
```c++
Core()->getOffset();
```

### Calling a radare2 command

There are two ways to do it:
* `CutterCore::cmd()` *(Discouraged)* Only use it for commands which yells no output
* `CutterCore::cmdj()` To be used with json commands like `cmdj("agj")` or `cmdj("aflj")`. It is way easier to parse a json output.

Example:
```c++
QJsonArray array = Core()->cmdj("pdj 1 @ main").array();
```

### Seek the current file

To modify radare2 seek use `CutterCore::seek(const RVA offset)`. This is important because it will emit a `CutterCore::seekChanged(RVA offset)` signal.
Never ever call `cmd("s offset")`;

Example:
```c++
Core()->seek(0xdeadbeef);
```

### Creating a widget

Make sure to connect the `CutterCore::seekChanged(RVA offset)` signal so your widget refreshes its output when radare2 seek is modified (switching to another function, etc.).

## General coding guidelines

### Coding style

We follow [these guidelines](https://wiki.qt.io/Qt_Coding_Style) to format the code.
If in doubt, you can use [AStyle 2.06](https://sourceforge.net/projects/astyle/files/astyle/astyle%202.06/) to format the code. The command line for formatting the code according to the style is:

```bash
astyle --project=src/Cutter.astylerc src/filename.cpp
```

#### Loops

We use C++11 foreach loop style which means any "foreach" loop should look like:
```c++
for (QJsonValue value : importsArray) {
    doSomething(value);
}
```

#### Nullptr

Please do not use `0` nor `Q_NULLPTR`, only use `nullptr`.

Example:
```c++
QObject *object = nullptr;
```

#### Connecting signals

To connect a signal to a slot, this is the preferred way to do it:
```c++
connect(sender, &QObject::destroyed, this, &MyObject::objectDestroyed);
```
The main reason is that this syntax allows the use of lambda functions.

### Functions documentation

It's good to add some documentation to your functions when needed. To do so we follow these [rules](http://doc.qt.io/qt-5/qdoc-guide-writing.html).

