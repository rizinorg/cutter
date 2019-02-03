#ifndef QTRESIMPORTER_H
#define QTRESIMPORTER_H

#ifdef CUTTER_ENABLE_PYTHON

PyObject *PyInit_qtres();

PyObject *QtResImport(const char *name);

#define RegQtResImporter() Py_DecRef(QtResImport("reg_qtres_importer"))

#endif // CUTTER_ENABLE_PYTHON

#endif // QTRESIMPORTER_H
