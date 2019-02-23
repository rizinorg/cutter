#ifdef CUTTER_ENABLE_PYTHON

#include <Python.h>
#include <marshal.h>

#include "QtResImporter.h"

#include <QFile>
#include <QDebug>

int QtResExists(const char *name, QFile &file)
{
    QString fname = QString::asprintf(":/python/%s.py", name);
    file.setFileName(fname);
    if (file.exists())
        return 1;
    fname.append('c');
    file.setFileName(fname);
    if (file.exists())
        return 2;
    return 0;
}

PyObject *QtResGetCode(const char *name)
{
    QFile moduleFile;
    bool isBytecode = false;

    switch (QtResExists(name, moduleFile)) {
    case 0:
        return nullptr;
    case 2:
        isBytecode = true;
    }

    moduleFile.open(QIODevice::ReadOnly);
    QByteArray data = moduleFile.readAll();
    moduleFile.close();

    PyObject *codeObject;
    if (isBytecode) {
        codeObject = PyMarshal_ReadObjectFromString(data.constData() + 12,
                                                    data.size() - 12);
    } else {
        codeObject = Py_CompileString(data.constData(),
                                      moduleFile.fileName().toLocal8Bit().constData(),
                                      Py_file_input);
    }
    if (!codeObject) {
        qWarning() << "Couldn't unmarshal/compile " << moduleFile.fileName();
    }
    return codeObject;
}

PyObject *QtResImport(const char *name)
{
    PyObject *codeObject = QtResGetCode(name);
    if (!codeObject)
        return nullptr;
    PyObject *module = PyImport_ExecCodeModule(name, codeObject);
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
