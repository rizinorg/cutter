
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

PyObject *api_internal_kernel_interface_kill(PyObject *, PyObject *args)
{
    long id;

    if (!PyArg_ParseTuple(args, "l", &id))
    {
        qWarning() << "Invalid args passed to api_internal_kernel_interface_kill().";
        return nullptr;
    }

    Jupyter()->getNestedIPyKernel(id)->kill();

    Py_RETURN_NONE;
}

PyMethodDef CutterInternalMethods[] = {
    {"launch_ipykernel", (PyCFunction)api_internal_launch_ipykernel, METH_VARARGS | METH_KEYWORDS,
    "Launch an IPython Kernel in a subinterpreter"},
    {"kernel_interface_kill", (PyCFunction)api_internal_kernel_interface_kill, METH_VARARGS, ""},
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

