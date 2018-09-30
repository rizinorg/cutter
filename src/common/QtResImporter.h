#ifndef QTRESIMPORTER_H
#define QTRESIMPORTER_H

PyObject *PyInit_qtres();

PyObject *QtResImport(const char *name);

#define RegQtResImporter() Py_DecRef(QtResImport("reg_qtres_importer"))

#endif // QTRESIMPORTER_H
