#ifndef IOMODESCONTROLLER_H
#define IOMODESCONTROLLER_H

#include "core/Cutter.h"
#include <qwidget.h>

class IOModesController : public QObject

{
    Q_OBJECT
public:
    IOModesController(QWidget *parent);
    enum class Mode { READ_ONLY, CACHE, WRITE };
    bool prepareForWriting();
    bool canWrite();
    bool allChangesComitted();
    Mode getIOMode();
    void setIOMode(Mode mode);

public slots:
    bool askCommitUnsavedChanges();

private:
    QWidget *parentWindow;
};

#endif // IOMODESCONTROLLER_H
