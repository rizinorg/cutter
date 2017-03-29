#ifndef QHELPERS_H
#define QHELPERS_H

#include <QObject>
#include <QTextEdit>
#include <QPlainTextEdit>

#include "mainwindow.h"

class QHelpers : public QObject
{
    Q_OBJECT
public:
    explicit QHelpers(QObject *parent = 0);
    void normalizeFont(QPlainTextEdit *edit);
    void normalizeEditFont(QTextEdit *edit);

signals:

public slots:

};

#endif // HELPERS_H
