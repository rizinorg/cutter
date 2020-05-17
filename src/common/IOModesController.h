#ifndef IOMODESCONTROLLER_H
#define IOMODESCONTROLLER_H

#include "core/Cutter.h"

class IOModesController : public QObject

{
    Q_OBJECT
public:
    enum class Mode { READ_ONLY, CACHE, WRITE };
    bool prepareForWriting();
    bool canWrite();
    bool allChangesComitted();
    Mode getIOMode();

public slots:
    bool askCommitUnsavedChanges();
};

#endif // IOMODESCONTROLLER_H
