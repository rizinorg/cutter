#include "PythonAPI.h"
#include "cutter.h"

/* Return the number of arguments of the application command line */
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
