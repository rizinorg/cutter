#ifndef PYTHONAPI_H
#define PYTHONAPI_H

#ifdef CUTTER_ENABLE_PYTHON

#include <Python.h>

PyObject *PyInit_api();
PyObject *PyInit_api_internal();

#endif

#endif // PYTHONAPI_H
