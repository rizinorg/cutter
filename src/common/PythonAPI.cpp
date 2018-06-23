
#include "PythonAPI.h"
#include "Cutter.h"

#include "CutterConfig.h"

#include <QFile>

PyObject *api_version(PyObject *self, PyObject *null)
{
    Q_UNUSED(self)
    Q_UNUSED(null)
    return PyUnicode_FromString("Cutter version " CUTTER_VERSION_FULL);
}

PyObject *api_cmd(PyObject *self, PyObject *args)
{
    Q_UNUSED(self);
    char *command;
    char *result = (char *) "";
    QString cmdRes;
    QByteArray cmdBytes;
    if (PyArg_ParseTuple(args, "s:command", &command)) {
        cmdRes = Core()->cmd(command);
        cmdBytes = cmdRes.toLocal8Bit();
        result = cmdBytes.data();
    }
    return PyUnicode_FromString(result);
}

PyObject *api_refresh(PyObject *self, PyObject *args)
{
    Q_UNUSED(self);
    Q_UNUSED(args);
    Core()->triggerRefreshAll();
    return Py_None;
}

PyObject *api_message(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Q_UNUSED(self);
    char *message;
    int debug = 0;
    static const char *kwlist[] = { "", "debug", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i",
                                     const_cast<char**>(kwlist),
                                     &message, &debug)) {
        return NULL;
    }
    Core()->message(QString(message), debug);
    Py_INCREF(Py_None);
    return Py_None;
}

PyMethodDef CutterMethods[] = {
    {
        "version", api_version, METH_NOARGS,
        "Returns Cutter current version"
    },
    {
        "cmd", api_cmd, METH_VARARGS,
        "Execute a command inside Cutter"
    },
    {
        "refresh", api_refresh, METH_NOARGS,
        "Refresh Cutter widgets"
    },
    {
        "message", (PyCFunction) api_message, METH_VARARGS | METH_KEYWORDS,
        "Print message"
    },
    {NULL, NULL, 0, NULL}
};

PyModuleDef CutterModule = {
    PyModuleDef_HEAD_INIT, "_cutter", NULL, -1, CutterMethods,
    NULL, NULL, NULL, NULL
};

PyObject *PyInit_api()
{
    return PyModule_Create(&CutterModule);
}

// -----------------------------

#ifdef CUTTER_ENABLE_JUPYTER
#include "JupyterConnection.h"
#include "NestedIPyKernel.h"

PyObject *api_internal_launch_ipykernel(PyObject *self, PyObject *args, PyObject *kw)
{
    Q_UNUSED(self);
    Q_UNUSED(kw);

    QStringList argv;
    PyObject *argvListObject;

    if (!PyArg_ParseTuple(args, "O", &argvListObject)
            || !PyList_Check(argvListObject)) {
        const char *msg = "Invalid args passed to api_internal_launch_ipykernel().";
        qWarning() << msg;
        PyErr_SetString(PyExc_RuntimeError, msg);
        return nullptr;
    }

    for (int i = 0; i < PyList_Size(argvListObject); i++) {
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

    if (!PyArg_ParseTuple(args, "ll", &id, &signum)) {
        const char *msg = "Invalid args passed to api_internal_kernel_interface_send_signal().";
        qWarning() << msg;
        PyErr_SetString(PyExc_RuntimeError, msg);
        return nullptr;
    }

    NestedIPyKernel *kernel = Jupyter()->getNestedIPyKernel(id);
    if (kernel) {
        kernel->sendSignal(signum);
    }

    Py_RETURN_NONE;
}

PyObject *api_internal_kernel_interface_poll(PyObject *, PyObject *args)
{
    long id;

    if (!PyArg_ParseTuple(args, "l", &id)) {
        const char *msg = "Invalid args passed to api_internal_kernel_interface_poll().";
        qWarning() << msg;
        PyErr_SetString(PyExc_RuntimeError, msg);
        return nullptr;
    }

    QVariant v = Jupyter()->pollNestedIPyKernel(id);
    bool ok;
    auto ret = static_cast<long>(v.toLongLong(&ok));
    if (ok) {
        return PyLong_FromLong(ret);
    } else {
        Py_RETURN_NONE;
    }
}

PyObject *api_internal_thread_set_async_exc(PyObject *, PyObject *args)
{
    long id;
    PyObject *exc;

    if (!PyArg_ParseTuple(args, "lO", &id, &exc)) {
        const char *msg = "Invalid args passed to api_internal_thread_set_async_exc().";
        qWarning() << msg;
        PyErr_SetString(PyExc_RuntimeError, msg);
        return nullptr;
    }

    int ret = PyThreadState_SetAsyncExc(id, exc);
    return PyLong_FromLong(ret);
}

PyMethodDef CutterInternalMethods[] = {
    {
        "launch_ipykernel", reinterpret_cast<PyCFunction>((void *)api_internal_launch_ipykernel), METH_VARARGS | METH_KEYWORDS,
        "Launch an IPython Kernel in a subinterpreter"
    },
    {"kernel_interface_send_signal", (PyCFunction)api_internal_kernel_interface_send_signal, METH_VARARGS, ""},
    {"kernel_interface_poll", (PyCFunction)api_internal_kernel_interface_poll, METH_VARARGS, ""},
    {"thread_set_async_exc", (PyCFunction)api_internal_thread_set_async_exc, METH_VARARGS, ""},
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

#endif // CUTTER_ENABLE_JUPYTER
