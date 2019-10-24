#ifndef PYTHONAPI_H
#define PYTHONAPI_H

#ifdef CUTTER_ENABLE_PYTHON

#define Py_LIMITED_API 0x03050000
#include <Python.h>

PyObject *PyInit_api();

#endif

#endif // PYTHONAPI_H
