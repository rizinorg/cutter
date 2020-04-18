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
    // connect(ui->saveRC, SIGNAL(accepted()), this, &InitializationFileEditor::saveCutterRC);
    connect(ui->saveRC, &QDialogButtonBox::accepted, this, &InitializationFileEditor::saveCutterRC);
    // onnect(ui->saveButtons,
    //         SIGNAL(accepted()),
    //         this, SLOT(close()));
    // connect(ui->actionNew, &QAction::triggered, this, &Notepad::newDocument);
    const QDir cutterRCDirectory = Core()->getCutterRCDefaultDirectory();
    // const auto cutterRCDirectory = CutterCore::getCutterRCDirectories();
    auto cutterRCFileInfo = QFileInfo(cutterRCDirectory, ".cutterrc");
    QString cutterRCLocation = cutterRCFileInfo.absoluteFilePath();
    // if (!cutterRCFileInfo.isFile()) {

    // }
    ui->ConfigFileEdit->clear();
    QFile cutterRC(cutterRCLocation);
    if (cutterRC.open(QIODevice::ReadWrite | QFile::Text)){ 

        ui->ConfigFileEdit->setPlainText(cutterRC.readAll()); 
    }
    cutterRC.close();

    // if (cutterRC.open(QIODevice::ReadOnly | QIODevice::Text)){
    //     QTextStream stream(&cutterRC);
    //     while (!stream.atEnd()){
    //         cutterRC = stream.readLine();
    //         ui->ConfigFileEdit->setText(ui->ConfigFileEdit->toPlainText()+cutterRC+"\n");
    //         qDebug() << "linea: "<<line;
    //     }
    // }
}

InitializationFileEditor::~InitializationFileEditor() {};


void InitializationFileEditor::saveCutterRC(){
    const QDir cutterRCDirectory = Core()->getCutterRCDefaultDirectory();
    // const auto cutterRCDirectory = CutterCore::getCutterRCDirectories();
    auto cutterRCFileInfo = QFileInfo(cutterRCDirectory, ".cutterrc");
    QString cutterRCLocation = cutterRCFileInfo.absoluteFilePath();
    // if (!cutterRCFileInfo.isFile()) {

    // }
    // ui->ConfigFileEdit->clear();
    QFile cutterRC(cutterRCLocation);
    // QTextStream out(&cutterRC);
    if (cutterRC.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)){ 
        QTextStream out(&cutterRC);
        //QTextStream out(&cutterRC, QIODevice::ReadWrite | QIODevice::Truncate);
        //ui->ConfigFileEdit->setPlainText(cutterRC.readAll());
        QString text = ui->ConfigFileEdit->toPlainText();
        //out.flush();
        out << text;
        cutterRC.close();
    }
    // QString text = ui->ConfigFileEdit->toPlainText();
    // out << text;
    // cutterRC.close();
} 