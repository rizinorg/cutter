#ifndef CUTTERAPPLICATION_H
#define CUTTERAPPLICATION_H

#include <QEvent>
#include <QApplication>
#include <QList>

#include "MainWindow.h"


class CutterApplication : public QApplication
{
    Q_OBJECT

public:
    CutterApplication(int &argc, char **argv);
    ~CutterApplication();

    MainWindow *getMainWindow()
    {
        return mainWindow;
    }

    void loadPlugins();

protected:
    bool event(QEvent *e);

private:
    bool m_FileAlreadyDropped;
    MainWindow *mainWindow;
};

#endif // CUTTERAPPLICATION_H
