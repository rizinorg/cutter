#ifndef CUTTERAPPLICATION_H
#define CUTTERAPPLICATION_H

#include <QEvent>
#include <QApplication>
#include <QList>
#include <QProxyStyle>

#include "core/MainWindow.h"


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

protected:
    bool event(QEvent *e);

private:
    /**
     * @brief Load and translations depending on Language settings
     * @return true on success
     */
    bool loadTranslations();

private:
    bool m_FileAlreadyDropped;
    MainWindow *mainWindow;
};


/**
 * @brief CutterProxyStyle is used to force shortcuts displaying in context menu
 */
class CutterProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    /**
     * @brief it is enough to get notification about QMenu polishing to force shortcut displaying
     */
    void polish(QWidget *widget) override;
};

#endif // CUTTERAPPLICATION_H
