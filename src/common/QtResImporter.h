#ifndef QTRESIMPORTER_H
#define QTRESIMPORTER_H

#ifdef CUTTER_ENABLE_JUPYTER

PyObject *PyInit_qtres();

PyObject *QtResImport(const char *name);

#define RegQtResImporter() Py_DecRef(QtResImport("reg_qtres_importer"))

#endif // CUTTER_ENABLE_JUPYTER

#endif // QTRESIMPORTER_H
