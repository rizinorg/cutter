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
    void executeCutterRC();
    
private:
    std::unique_ptr<Ui::InitializationFileEditor> ui;

};


#endif //INITIALIZATIONFILEEDITOR_H