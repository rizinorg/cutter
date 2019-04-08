#ifdef CUTTER_ENABLE_PYTHON

#define Py_LIMITED_API 0x03050000
#include <Python.h>

#include "QtResImporter.h"

#include <QFile>
#include <QDebug>

bool QtResExists(const char *name, QFile &file)
{
    QString fname = QString::asprintf(":/python/%s.py", name);
    file.setFileName(fname);
    return file.exists();
}

PyObject *QtResGetCode(const char *name)
{
    QFile moduleFile;

    if (!QtResExists(name, moduleFile)) {
        return nullptr;
    }

    moduleFile.open(QIODevice::ReadOnly);
    QByteArray data = moduleFile.readAll();
    moduleFile.close();

    PyObject *codeObject = Py_CompileString(data.constData(),
                                            moduleFile.fileName().toLocal8Bit().constData(),
                                            Py_file_input);
    if (!codeObject) {
        qWarning() << "Couldn't compile " << moduleFile.fileName();
    }
    return codeObject;
}

PyObject *QtResImport(const char *name)
{
    PyObject *codeObject = QtResGetCode(name);
    if (!codeObject) {
        return nullptr;
    }
    PyObject *module = PyImport_ExecCodeModule(name, codeObject);
    if (!module) {
        PyErr_Print();
    }
    Py_DECREF(codeObject);
    return module;
}

PyObject *qtres_exists(PyObject *self, PyObject *args)
{
    Q_UNUSED(self)
    char *name;
    QFile resFile;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    return PyBool_FromLong(QtResExists(name, resFile));
}

PyObject *qtres_get_code(PyObject *self, PyObject *args)
{
    Q_UNUSED(self)
    char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    PyObject *ret = QtResGetCode(name);
    if (ret)
        return ret;
    Py_RETURN_NONE;
}

PyMethodDef QtResMethods[] = {
    { "exists", qtres_exists, METH_VARARGS, NULL },
    { "get_code", qtres_get_code, METH_VARARGS, NULL },
    {NULL, NULL, 0, NULL}
};

PyModuleDef QtResModule = {
    PyModuleDef_HEAD_INIT, "_qtres", NULL, -1, QtResMethods,
    NULL, NULL, NULL, NULL
};

PyObject *PyInit_qtres()
{
    return PyModule_Create(&QtResModule);
}

#endif // CUTTER_ENABLE_PYTHON
