#include "IOModesController.h"
#include "Cutter.h"

#include <QJsonArray>
#include <QPushButton>
#include <QObject>
#include <QMessageBox>
#include <QJsonObject>

bool IOModesController::canWrite()
{
    return Core()->isIOCacheEnabled() || Core()->isWriteModeEnabled();
}

IOModesController::Mode IOModesController::getIOMode()
{
    if (Core()->isWriteModeEnabled()) {
        return Mode::WRITE;
    } else if (Core()->isIOCacheEnabled()) {
        return Mode::CACHE;
    } else {
        return Mode::READ_ONLY;
    }
}

void IOModesController::setIOMode(IOModesController::Mode mode)
{
    switch (mode) {
    case Mode::READ_ONLY:
        if (askCommitUnsavedChanges()) {
            Core()->setWriteMode(false);
        }
        break;
    case Mode::CACHE:
        Core()->setIOCache(true);
        break;
    case Mode::WRITE:
        if (askCommitUnsavedChanges()) {
            Core()->setWriteMode(true);
        }
        break;
    }
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

bool IOModesController::allChangesComitted()
{
    // Get a list of available write changes
    QJsonArray changes = Core()->cmdj("wcj").array();

    // Check if there is a change which isn't written to the file
    for (const QJsonValue &value : changes) {
        QJsonObject changeObject = value.toObject();
        if (!changeObject["written"].toBool()) {
            return false;
        }
    }

    return true;
}

bool IOModesController::askCommitUnsavedChanges()
{
    // Check if there are uncommitted changes
    if (!allChangesComitted()) {
        QMessageBox::StandardButton ret = QMessageBox::question(NULL, QObject::tr("Uncomitted changes"),
                                                                QObject::tr("It seems that you have changes or patches that are not committed to the file.\n"
                                                                            "Do you want to commit them now?"),
                                                                (QMessageBox::StandardButtons)(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel));
        if (ret == QMessageBox::Save) {
            Core()->commitWriteCache();
        } else if (ret == QMessageBox::Discard) {
            Core()->cmdRaw("wcr");
            emit Core()->refreshCodeViews();
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
    }

    return true;
}
