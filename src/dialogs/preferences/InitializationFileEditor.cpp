#include <QLabel>
#include <QFontDialog>
#include <QTextEdit>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDialogButtonBox>

#include "InitializationFileEditor.h"
#include "ui_InitializationFileEditor.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

InitializationFileEditor::InitializationFileEditor(PreferencesDialog *dialog)
    : QDialog(dialog),
      ui(new Ui::InitializationFileEditor)
{
    ui->setupUi(this);
    connect(ui->saveRC, &QDialogButtonBox::accepted, this, &InitializationFileEditor::saveCutterRC);
    const QDir cutterRCDirectory = Core()->getCutterRCDefaultDirectory();
    if(!cutterRCDirectory.exists()){
        cutterRCDirectory.mkpath(".");
    }
    auto cutterRCFileInfo = QFileInfo(cutterRCDirectory, ".cutterrc");
    QString cutterRCLocation = cutterRCFileInfo.absoluteFilePath();
    ui->ConfigFileEdit->clear();
    QFile cutterRC(cutterRCLocation);
    if(cutterRC.open(QIODevice::ReadWrite | QIODevice::Text)){ 
        ui->ConfigFileEdit->setPlainText(cutterRC.readAll()); 
    }
    cutterRC.close();
}

InitializationFileEditor::~InitializationFileEditor() {};


void InitializationFileEditor::saveCutterRC(){
    const QDir cutterRCDirectory = Core()->getCutterRCDefaultDirectory();
    auto cutterRCFileInfo = QFileInfo(cutterRCDirectory, ".cutterrc");
    QString cutterRCLocation = cutterRCFileInfo.absoluteFilePath();
   
    QFile cutterRC(cutterRCLocation);
    if(cutterRC.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)){ 
        QTextStream out(&cutterRC);
        QString text = ui->ConfigFileEdit->toPlainText();
        out << text;
        cutterRC.close();
    }
} 