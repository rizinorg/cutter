#ifndef INITIALIZATIONFILEEDITOR_H
#define INITIALIZATIONFILEEDITOR_H

#include <QDialog>
#include <QPushButton>
#include <memory>
#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class InitializationFileEditor;
}

class InitializationFileEditor : public QDialog
{
    Q_OBJECT

public:
    explicit InitializationFileEditor(PreferencesDialog *dialog);
    ~InitializationFileEditor();
    void saveCutterRC();
private:
    std::unique_ptr<Ui::InitializationFileEditor> ui;


private slots:


};


#endif //INITIALIZATIONFILEEDITOR_H