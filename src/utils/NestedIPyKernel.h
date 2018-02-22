

#ifndef NESTEDIPYKERNEL_H
#define NESTEDIPYKERNEL_H

#include <QStringList>

struct _object;
typedef _object PyObject;

struct _ts;
typedef _ts PyThreadState;

class NestedIPyKernel
{
public:
    static NestedIPyKernel *start(const QStringList &argv);
    ~NestedIPyKernel();

    void kill();

private:
    NestedIPyKernel(PyObject *cutterIPykernelModule, const QStringList &argv);

    PyThreadState *threadState;
    PyObject *kernel;
};

#endif //NESTEDIPYKERNEL_H
