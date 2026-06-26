#ifndef IOMODESCONTROLLER_H
#define IOMODESCONTROLLER_H

#include "core/Cutter.h"

#include <qwidget.h>

/**
 * @brief Manages switching between Read-Only, Cache, and Write modes
 *
 * Handles the logic required when changing how Cutter interacts with the underlying file on disk
 */
class IOModesController : public QObject

{
    Q_OBJECT
public:
    IOModesController(QWidget *parent);
    enum class Mode { READ_ONLY, CACHE, WRITE };
    bool prepareForWriting();
    bool canWrite();
    Mode getIOMode();
    void setIOMode(Mode mode);

public slots:
    bool askCommitUnsavedChanges();

private:
    QWidget *parentWindow;
};

#endif // IOMODESCONTROLLER_H
