#ifndef SUBBINARYSELECTORDIALOG_H
#define SUBBINARYSELECTORDIALOG_H

#include "core/CutterDescriptions.h"

#include <QDialog>
#include <QList>

#include <memory>

namespace Ui {
class SubBinarySelectorDialog;
}

/**
 * @brief Dialog that lets the user pick one binary from a container file
 * (e.g. a fat Mach-O) that holds several binaries.
 */
class SubBinarySelectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubBinarySelectorDialog(const QString &filename,
                                     const QList<SubBinaryDescription> &subBinaries,
                                     QWidget *parent = nullptr);
    ~SubBinarySelectorDialog();

    /**
     * @brief The binary the user picked. Only valid after exec() returned Accepted.
     */
    SubBinaryDescription getSelectedSubBinary() const;

private:
    std::unique_ptr<Ui::SubBinarySelectorDialog> ui;
    QList<SubBinaryDescription> subBinaries;
};

#endif // SUBBINARYSELECTORDIALOG_H