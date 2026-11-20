#ifndef SUBBINARYDIALOG_H
#define SUBBINARYDIALOG_H

#include "core/CutterDescriptions.h"

#include <QButtonGroup>
#include <QDialog>
#include <QList>

class SubBinaryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubBinaryDialog(const QList<SubBinaryDescription> &subBinaries,
                             QWidget *parent = nullptr);
    int selectedIndex() const;

private:
    QButtonGroup *buttonGroup = nullptr;
};

#endif // SUBBINARYDIALOG_H