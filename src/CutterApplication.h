#ifndef CUTTERAPPLICATION_H
#define CUTTERAPPLICATION_H

#include <QEvent>
#include <QApplication>

#include "MainWindow.h"


class CutterApplication : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(MainWindow* mainWindow READ mainWindow WRITE setMainWindow)

public:
    CutterApplication(int &argc, char **argv);

    MainWindow * mainWindow() {
        return m_MainWindow;
    }

    void setMainWindow(MainWindow * mw) {
        m_MainWindow = mw;
    }

protected:
    bool event(QEvent *e);

private:
    bool m_FileAlreadyDropped;
    MainWindow *m_MainWindow;
};

#endif // CUTTERAPPLICATION_H
