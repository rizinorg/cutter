#include "IOModesController.h"
#include "Cutter.h"

#include <QPushButton>
#include <QObject>

bool IOModesController::canWrite()
{
    return Core()->isIOCacheEnabled() || Core()->isWriteModeEnabled();
}

bool IOModesController::prepareForWriting()
{
    if (canWrite()) {
        return true;
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle(QObject::tr("Write error"));
    msgBox.setText(
        QObject::tr("Your file is opened in read-only mode. "
           "Editing is only available when the file is opened in either Write or Cache modes.\n\n"
           "WARNING: In Write mode, any changes will be committed to the file on disk. "
           "For safety, please consider using Cache mode and then commit the changes manually "
           "via File -> Commit modifications to disk."));
    msgBox.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
    QAbstractButton *reopenButton = msgBox.addButton(QObject::tr("Reopen in Write mode"),
                                                     QMessageBox::YesRole);
    QAbstractButton *iocacheButton = msgBox.addButton(QObject::tr("Enable Cache mode"),
                                                     QMessageBox::YesRole);

    msgBox.exec();

    if (msgBox.clickedButton() == reopenButton) {
        Core()->setWriteMode(true);
    } else if (msgBox.clickedButton() == iocacheButton) {
        Core()->setIOCache(true);
    } else {
        return false;
    }
    return true;
}
