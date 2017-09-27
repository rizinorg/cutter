# Common errors

Please make sure you have the appropriate QT version: [https://www.qt.io/qt5-6/](https://www.qt.io/qt5-6/)

> r_core development package not found

If you installed radare2 and still encounter this error, could be your `PATH` environment variable is set improperly (doesn't contain `/usr/local/bin`). That can be, for example, due to `Qt Creator.app` being copied over to `/Applications`.
To fix this, append:

> :/usr/local/bin

to the `PATH` variable within the *Build Environment* section in Qt Creator. See the screenshot below should you encounter any problems.

![PATH variable settings](https://d0vine.github.io/images/iaito_settings.png)

## Windows

See [Compiling on Windows](https://github.com/hteso/iaito/wiki/Compiling-on-Windows).