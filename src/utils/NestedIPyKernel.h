
#ifndef NESTEDIPYKERNEL_H
#define NESTEDIPYKERNEL_H

#ifdef CUTTER_ENABLE_JUPYTER

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

    void sendSignal(long signum);
    QVariant poll();

    PyThreadState *getThreadState()
    {
        return threadState;
    }

private:
    NestedIPyKernel(PyObject *cutterIPykernelModule, const QStringList &argv);

    PyThreadState *threadState;
    PyObject *kernel;
};

#endif

#endif //NESTEDIPYKERNEL_H
