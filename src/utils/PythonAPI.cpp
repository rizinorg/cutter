
#include "PythonAPI.h"
#include "cutter.h"

#include <QFile>

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


// -----------------------------


PyObject *api_internal_launch_ipykernel(PyObject *self, PyObject *args, PyObject *kw)
{
    QStringList argv;

    {
        PyObject *argvListObject;

        if (!PyArg_ParseTuple(args, "O", &argvListObject)
                || !PyList_Check(argvListObject))
        {
            qWarning() << "Invalid args passed to api_internal_launch_ipykernel().";
            return nullptr;
        }

        for (int i=0; i<PyList_Size(argvListObject); i++)
        {
            PyObject *o = PyList_GetItem(argvListObject, i);
            QString s = QString::fromUtf8(PyUnicode_AsUTF8(o));
            argv.append(s);
        }
    }


    PyThreadState *parentThreadState = PyThreadState_Get();

    PyThreadState *threadState = Py_NewInterpreter();

    QFile moduleFile(":/python/cutter_ipykernel.py");
    moduleFile.open(QIODevice::ReadOnly);
    QByteArray moduleCode = moduleFile.readAll();
    moduleFile.close();

    auto moduleCodeObject = Py_CompileString(moduleCode.constData(), "cutter_ipykernel.py", Py_file_input);
    if (!moduleCodeObject)
    {
        qWarning() << "Could not compile cutter_ipykernel.";
        return nullptr;
    }
    auto cutterIPykernelModule = PyImport_ExecCodeModule("cutter_ipykernel", moduleCodeObject);
    Py_DECREF(moduleCodeObject);
    if (!cutterIPykernelModule)
    {
        qWarning() << "Could not import cutter_ipykernel.";
        return nullptr;
    }

    auto launchFunc = PyObject_GetAttrString(cutterIPykernelModule, "launch_ipykernel");

    PyObject *argvListObject = PyList_New(argv.size());
    for (int i=0; i<argv.size(); i++)
    {
        QString s = argv[i];
        PyList_SetItem(argvListObject, i, PyUnicode_DecodeUTF8(s.toUtf8().constData(), s.length(), nullptr));
    }


    auto ipyKernel = PyObject_CallFunction(launchFunc, "O", argvListObject);

    PyThreadState_Swap(parentThreadState);

    return PyLong_FromLong(42);
}

PyMethodDef CutterInternalMethods[] = {
    {"launch_ipykernel", (PyCFunction)api_internal_launch_ipykernel, METH_VARARGS | METH_KEYWORDS,
    "Launch an IPython Kernel in a subinterpreter"},
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

