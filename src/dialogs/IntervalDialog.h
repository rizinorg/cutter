#ifndef INTERVALDIALOG_H
#define INTERVALDIALOG_H

#include "CutterCommon.h"

#include <QDialog>

Q_DECLARE_METATYPE(RzInterval)

class QLineEdit;

/**
 * @brief Dialog for taking input in interval format which includes starting address and size
 *
 * @see RizinConfigOptionsDelegate
 */
class IntervalDialog : public QDialog
{
    Q_OBJECT

public:
    IntervalDialog(const QString &name, QWidget *parent = nullptr);
    void setAddress(ut64 addr);
    void setSize(ut64 size);

    QString getAddress() const;
    QString getSize() const;

private:
    QLineEdit *addressEdit;
    QLineEdit *sizeEdit;
};

#endif // INTERVALDIALOG_H