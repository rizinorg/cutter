#include <QLabel>
#include <QFontDialog>
#include <QTextEdit>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QUrl>

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
    connect(ui->executeNow, &QDialogButtonBox::accepted, this, &InitializationFileEditor::executeCutterRC);
    connect(ui->ConfigFileEdit, &QPlainTextEdit::modificationChanged, ui->saveRC, &QWidget::setEnabled);

    const QDir cutterRCDirectory = Core()->getCutterRCDefaultDirectory();
    auto cutterRCFileInfo = QFileInfo(cutterRCDirectory, "rc");
    QString cutterRCLocation = cutterRCFileInfo.absoluteFilePath();
    
    ui->cutterRCLoaded->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->cutterRCLoaded->setOpenExternalLinks(true);
    ui->cutterRCLoaded->setText(tr("Script is loaded from <a href=\"%1\">%2</a>")
                                .arg(QUrl::fromLocalFile(cutterRCDirectory.absolutePath()).toString(), cutterRCLocation.toHtmlEscaped()));
    
    ui->executeNow->button(QDialogButtonBox::Retry)->setText("Execute");
    ui->ConfigFileEdit->clear();
    if(cutterRCFileInfo.exists()){
        QFile cutterRC(cutterRCLocation);
        if(cutterRC.open(QIODevice::ReadWrite | QIODevice::Text)){ 
            ui->ConfigFileEdit->setPlainText(cutterRC.readAll()); 
        }
        cutterRC.close();
    }
    ui->saveRC->setDisabled(true);
}

InitializationFileEditor::~InitializationFileEditor() {};


void InitializationFileEditor::saveCutterRC(){
    const QDir cutterRCDirectory = Core()->getCutterRCDefaultDirectory();
    if(!cutterRCDirectory.exists()){
        cutterRCDirectory.mkpath(".");
    }
    auto cutterRCFileInfo = QFileInfo(cutterRCDirectory, "rc");
    QString cutterRCLocation = cutterRCFileInfo.absoluteFilePath();
   
    QFile cutterRC(cutterRCLocation);
    if(cutterRC.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)){ 
        QTextStream out(&cutterRC);
        QString text = ui->ConfigFileEdit->toPlainText();
        out << text;
        cutterRC.close();
    }
    ui->ConfigFileEdit->document()->setModified(false);
}

void InitializationFileEditor::executeCutterRC(){
    saveCutterRC();
    Core()->loadDefaultCutterRC();
}
