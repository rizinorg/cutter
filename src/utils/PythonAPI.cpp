
#include "PythonAPI.h"
#include "cutter.h"
#include "JupyterConnection.h"
#include "NestedIPyKernel.h"

#include <QFile>

PyObject *api_version(PyObject *self, PyObject *null)
{
    Q_UNUSED(self)
    Q_UNUSED(null)
    return PyUnicode_FromString("Cutter version " CUTTER_VERSION);
}

PyObject *api_cmd(PyObject *self, PyObject *args)
{
    Q_UNUSED(self);
    char *command;
    char *result = (char*) "";
    if (PyArg_ParseTuple(args, "s:command", &command))
    {
        result = Core()->cmd(command).toUtf8().data();
    }
    return PyUnicode_FromString(result);
}

PyMethodDef CutterMethods[] = {
    {"version", api_version, METH_NOARGS,
     "Returns Cutter current version"},
    {"cmd", api_cmd, METH_VARARGS,
     "Execute a command inside Cutter"},
    {NULL, NULL, 0, NULL}
};

PyModuleDef CutterModule = {
    PyModuleDef_HEAD_INIT, "cutter", NULL, -1, CutterMethods,
    NULL, NULL, NULL, NULL
};

PyObject *PyInit_api()
{
    return PyModule_Create(&CutterModule);
}


// -----------------------------


PyObject *api_internal_launch_ipykernel(PyObject *self, PyObject *args, PyObject *kw)
{
    Q_UNUSED(self);
    Q_UNUSED(kw);

    QStringList argv;
    PyObject *argvListObject;

    if (!PyArg_ParseTuple(args, "O", &argvListObject)
            || !PyList_Check(argvListObject))
    {
        qWarning() << "Invalid args passed to api_internal_launch_ipykernel().";
        return nullptr;
    }

    for (int i = 0; i < PyList_Size(argvListObject); i++)
    {
        PyObject *o = PyList_GetItem(argvListObject, i);
        QString s = QString::fromUtf8(PyUnicode_AsUTF8(o));
        argv.append(s);
    }

    long id = Jupyter()->startNestedIPyKernel(argv);

    return PyLong_FromLong(id);
}

PyObject *api_internal_kernel_interface_send_signal(PyObject *, PyObject *args)
{
    long id;
    long signum;

    if (!PyArg_ParseTuple(args, "ll", &id, &signum))
    {
        qWarning() << "Invalid args passed to api_internal_kernel_interface_send_signal().";
        return nullptr;
    }

    NestedIPyKernel *kernel = Jupyter()->getNestedIPyKernel(id);
    if(kernel)
    {
        kernel->sendSignal(signum);
    }

    Py_RETURN_NONE;
}

PyObject *api_internal_kernel_interface_poll(PyObject *, PyObject *args)
{
    long id;

    if (!PyArg_ParseTuple(args, "l", &id))
    {
        qWarning() << "Invalid args passed to api_internal_kernel_interface_poll().";
        return nullptr;
    }

    NestedIPyKernel *kernel = Jupyter()->getNestedIPyKernel(id);
    if(!kernel)
    {
        return PyLong_FromLong(0);
    }

    QVariant v = kernel->poll();
    bool ok;
    auto ret = static_cast<long>(v.toLongLong(&ok));
    if(ok)
    {
        return PyLong_FromLong(ret);
    }
    else
    {
        Py_RETURN_NONE;
    }
}

PyMethodDef CutterInternalMethods[] = {
    {"launch_ipykernel", (PyCFunction)api_internal_launch_ipykernel, METH_VARARGS | METH_KEYWORDS,
    "Launch an IPython Kernel in a subinterpreter"},
    {"kernel_interface_send_signal", (PyCFunction)api_internal_kernel_interface_send_signal, METH_VARARGS, ""},
    {"kernel_interface_poll", (PyCFunction)api_internal_kernel_interface_poll, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

PyModuleDef CutterInternalModule = {
    PyModuleDef_HEAD_INIT, "cutter_internal", NULL, -1, CutterInternalMethods,
    NULL, NULL, NULL, NULL
};


PyObject *PyInit_api_internal()
{
    return PyModule_Create(&CutterInternalModule);
}

