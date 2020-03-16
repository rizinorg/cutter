#ifndef IOMODESCONTROLLER_H
#define IOMODESCONTROLLER_H

#include "core/Cutter.h"

class IOModesController
{


public:
    bool prepareForWriting();
    bool canWrite();
};

#endif // IOMODESCONTROLLER_H
